/*
 * Basic routine for parsing topology of a machine. Read the topology 
 * automatically from the Operating System or user supplied configuration file.
 *
 * For simplicity of implementation, I assume that the processors in a machine 
 * are identical, i.e., each socket has the same number of
 * nodes, and each node has the same number of cores.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <common_toolx.h>

#include "reeact_utils.h"


/*
 * some macros for simplifying the reading of files
 */
#define OPEN_FILE(fp, fname, err)					\
	do{								\
		fp = fopen(fname, "r");					\
		if(fp == NULL) {					\
			LOGERRX("Unable to open file %s: ", fname);	\
			return err;					\
		}							\
	} while(0);

#define GET_LINE_MUST_SUCCEED(fp, line, len, size, fname, err)		\
	do{								\
		len = getline(&line, &size, fp);			\
		if(len == -1){						\
			LOGERRX("Unable to read line from file %s: ");	\
			return err;					\
		}							\
		if(line[len-1] == '\n')					\
			line[len-1] = '\0';				\
	} while(0);

/*
 * For Linux system, get topology from sysfs.
 * Input parameters and return values are the same as reeact_get_topology.
 */
#define NODE_INFO_DIRECTORY "/sys/devices/system/node/node"
#define NODE_CORE_LIST_FILE "cpulist"
#define CPU_INFO_DIRECTORY "/sys/devices/system/cpu/cpu"
#define CPU_CONTEXT_LIST_FILE "topology/thread_siblings_list"
#define CPU_PACKAGE_ID_FILE "topology/physical_package_id"
#define ONLINE_NODE_LIST "/sys/devices/system/node/online"
#define ONLINE_CPU_LIST "/sys/devices/system/cpu/online"
int reeact_get_topo_sysfs(int **nodes, int **cores, int *socket_cnt, 
			int *node_cnt, int *core_cnt)
{
	int total_node_cnt, total_core_cnt, total_ctx_cnt;
	char filename[256] = {0};
	FILE *fp;
	char *buf = NULL;
	size_t buf_size;
	int ln_len;
	int ret_val;
	int *node_ids;
	int *ctx_ids;
	int i,j,k;
	int *contexts, ctx_cnt;

	/*
	 * get the total number of nodes
	 */
	OPEN_FILE(fp, ONLINE_NODE_LIST, 2);
	GET_LINE_MUST_SUCCEED(fp, buf, ln_len, buf_size, ONLINE_NODE_LIST, 2);
	ret_val = parse_value_list_expand(buf, (void**)&node_ids, 
					  &total_node_cnt, 0);

	if(ret_val){
		LOGERR("Unable to parse online node list, error %d\n", ret_val);
		return 2;
	}
#ifdef _REEACT_DEBUG_
	{
		DPRINTF("There are %d nodes: ", total_node_cnt);
		for(i = 0; i < total_node_cnt; i++){
			fprintf(stderr, "%d,", node_ids[i]);
		}
		fprintf(stderr,"\n");
	}
#endif
	/*
	 * get the total number of cpu SMT context
	 */
	OPEN_FILE(fp, ONLINE_CPU_LIST, 2);
	GET_LINE_MUST_SUCCEED(fp, buf, ln_len, buf_size, ONLINE_CPU_LIST, 2);
	ret_val = parse_value_list_expand(buf, (void**)&ctx_ids, 
					  &total_ctx_cnt, 0);

	if(ret_val){
		LOGERR("Unable to parse online cpu list, error %d\n", ret_val);
		return 2;
	}
	/*
	 * determine the real physical cores, here I assume the thread ids are
	 * sequential (no gap between thread ids). I reuse the ctx_ids array.
	 * if a cpu context i is a core, ctx_ids[i] is -1, otherwise 
	 * ctx_ids[i] is -2.
	 */
	total_core_cnt = 0;
	for(i = 0; i < total_ctx_cnt; i++){
		if(ctx_ids[i] == -2)
			continue; // this thread is known to be SMT context
		/* parse the cpu context file */
		sprintf(filename, "%s%d/%s", CPU_INFO_DIRECTORY, i, 
			CPU_CONTEXT_LIST_FILE);
		OPEN_FILE(fp, filename, 2);
		GET_LINE_MUST_SUCCEED(fp, buf, ln_len, buf_size, filename, 2);
		ret_val = parse_value_list_expand(buf, (void**)&contexts, 
						  &ctx_cnt, 0);
		if(ret_val){
			LOGERR("Unable to parse cpu %d's contexts , error %d\n",
			       i, ret_val);
			return 2;
		}
		/* mark this context as physical core */
		ctx_ids[i] = -1; 
		total_core_cnt++;
		/* mark other SMT contexts as SMT context */
		for(j = 0; j < ctx_cnt; j++){
			if(contexts[j] != i)
				ctx_ids[contexts[j]] = -2;
		}
		/* cleanup */
		if(contexts) 
			free(contexts);
	}

#ifdef _REEACT_DEBUG_
	{
		DPRINTF("There are %d cpu cores: ", total_core_cnt);
		for(i = 0; i < total_ctx_cnt; i++){
			if(ctx_ids[i] == -1)
				fprintf(stderr, "%d,", i);
		}
		fprintf(stderr,"\n");
	}
#endif
	/*
	 * map physical cores to nodes. Assuming nodes are homogeneous, and 
	 * there is no gap in node ids.
	 */
	*cores = (int*)malloc(total_core_cnt * sizeof(int));
	*core_cnt = total_core_cnt / total_node_cnt;
	DPRINTF("cores per node: %d\n", *core_cnt);
	if(*cores == NULL){
		LOGERRX("Unable to allocate space for core to node mapping: ");
		return 3;
	}
	for(i = 0; i < total_node_cnt; i++){
		/* parse the cpu list file */
		sprintf(filename, "%s%d/%s", NODE_INFO_DIRECTORY, i, 
			NODE_CORE_LIST_FILE);
		OPEN_FILE(fp, filename, 2);
		GET_LINE_MUST_SUCCEED(fp, buf, ln_len, buf_size, filename, 2);
		ret_val = parse_value_list_expand(buf, (void**)&contexts, 
						  &ctx_cnt, 0);
		if(ret_val){
			LOGERR("Unable to parse node %d's contexts, error %d\n",
			       i, ret_val);
			return 2;
		}
		/* assign core ids to node */
		k = 0;
		for(j = 0; j < ctx_cnt; j++){
			int ctx_id = contexts[j];
			if(ctx_ids[ctx_id] == -1){
				(*cores)[i * *core_cnt + k] = ctx_id;
				k++;
			}
		}
		/* cleanup */
		if(contexts != NULL)
			free(contexts);
	}

#ifdef _REEACT_DEBUG_
	{
		for(i = 0; i < total_node_cnt; i++){
			DPRINTF("node %d core list: ", i);
			for(j = 0; j < *core_cnt; j++){
				fprintf(stderr, "%d,", 
					(*cores)[i * *core_cnt + j]);
			}
			fprintf(stderr,"\n");
		}
	}
#endif

	/*
	 * determine the number of sockets
	 */
	*socket_cnt = -1;
	for(i = 0; i < total_node_cnt; i++){
		int core_id;
		/* id of the first core on node i */
		core_id = (*cores)[i * *core_cnt];
		
		/* parse the physical package id */
		sprintf(filename, "%s%d/%s", CPU_INFO_DIRECTORY, core_id, 
			CPU_PACKAGE_ID_FILE);
		OPEN_FILE(fp, filename, 2);
		GET_LINE_MUST_SUCCEED(fp, buf, ln_len, buf_size, filename, 2);
		ret_val = parse_value_list_expand(buf, (void**)&contexts, 
						  &ctx_cnt, 0);
		if(ret_val){
			LOGERR("Unable to parse cpu %d's package id, error "
			       "%d\n", i, ret_val);
			return 2;
		}

		if(contexts[0] > *socket_cnt)
			*socket_cnt = contexts[0];

		/* cleanup */
		if(contexts != NULL)
			free(contexts);
	}
	(*socket_cnt)++;
	DPRINTF("There are %d sockets\n", *socket_cnt);
	
	/* 
	 * map nodes to socket ids 
	 */
	*nodes = (int*)malloc(total_node_cnt*sizeof(int));
	*node_cnt = total_node_cnt / *socket_cnt;
	for(i = 0; i < *socket_cnt; i++){
		k = 0;
		for(j = 0; j < total_node_cnt; j++){
			int core_id, socket_id;
			/* id of the first core on node i */
			core_id = (*cores)[j * *core_cnt ];
			
			/* parse the physical package id */
			sprintf(filename, "%s%d/%s", CPU_INFO_DIRECTORY, 
				core_id, CPU_PACKAGE_ID_FILE);
			OPEN_FILE(fp, filename, 2);
			GET_LINE_MUST_SUCCEED(fp, buf, ln_len, buf_size, 
					      filename, 2);
			ret_val = parse_value_list_expand(buf, 
							  (void**)&contexts, 
							  &ctx_cnt, 0);
			if(ret_val){
				LOGERR("Unable to parse cpu %d's package id, "
				       "error %d\n",
				       i, ret_val);
				return 2;
			}
			
			/* id of the socket */
			socket_id = contexts[0];
			
			/* assign node to socket */
			if( socket_id == i){
				(*nodes)[socket_id * *node_cnt + k] = j;
				k++;
			}
			
			/* cleanup */
			if(contexts != NULL)
				free(contexts);
		}
	}
       
	/*
	 * cleanup 
	 */
	if(buf)
		free(buf);
	if(node_ids)
		free(node_ids);
	if(ctx_ids)
		free(ctx_ids);
	fclose(fp);

	return 0;
}


/*
 * Get the processor topology from user configuration file.
 * Input parameters and return values are the same as reeact_get_topology.
 */
int reeact_get_topo_conf(int **nodes, int **cores, int *socket_cnt, 
			 int *node_cnt, int *core_cnt)
{
	FILE *fp = NULL;
	char *buf = NULL;
	size_t buf_size;
	char *conf_file;
	char *p;
	int ln_len;
	int ret_val;
	int *counts;
	
	conf_file = getenv(REEACT_USER_TOPOLOGY_CONFIG);
	OPEN_FILE(fp, conf_file, 2);

	/* parse the file line by line */
	while((ln_len = getline(&buf, &buf_size, fp)) != -1){
		if(buf[ln_len-1] == '\n')
			buf[ln_len-1] = '\0';
		switch(buf[0]){
		case 's':
			/* socket, node, and core counts */
			p = buf;
			while((*p < '0' || *p > '9') && (*p != '\0'))
				p++;
			ret_val = parse_value_list_expand(p, 
							  (void**)&counts, 
							  &ln_len, 0);
			if(ret_val || ln_len != 3){
				LOGERR("Error parsing configure file line: "
				       "%s with error %d\n", p, ret_val);
				return 2;
			}
			
			*socket_cnt = counts[0];
			*node_cnt = counts[1];
			*core_cnt = counts[2];
			break;
		case 'n':
			/* nodes to sockets mapping array */
			p = buf;
			while((*p < '0' || *p > '9') && (*p != '\0'))
				p++;
			ret_val = parse_value_list_expand(p, 
							  (void**)nodes, 
							  &ln_len, 0);
			if(ret_val){
				LOGERR("Error parsing configure file line: "
				       "%s with error %d\n", buf, ret_val);
				return 2;
			}
			
			break;
		case 'c':
			/* nodes to sockets mapping array */
			p = buf;
			while((*p < '0' || *p > '9') && (*p != '\0'))
				p++;
			ret_val = parse_value_list_expand(p, 
							  (void**)cores, 
							  &ln_len, 0);
			if(ret_val){
				LOGERR("Error parsing configure file line: "
				       "%s with error %d\n", buf, ret_val);
				return 2;
			}
			
			break;
		default:
			break;
		}
	}

	/* check if we have all the information we need */
	if(*node_cnt == 0 || *core_cnt == 0 || *socket_cnt == 0 ||
	   *nodes == NULL || *cores == NULL){
		LOGERR("Topology configuration file %s is not complete.\n", 
		       conf_file);
		return 2;
	}

	/* cleanup */
	if(buf)
		free(buf);
	fclose(fp);

	return 0;
	
}

/*
 * Get the processor topology of current machine
 */
int reeact_get_topology(int **nodes, int **cores, int *socket_cnt, 
			int *node_cnt, int *core_cnt)
{
	int ret_val = 1;
	if(nodes == NULL || cores == NULL || socket_cnt == NULL ||
	   node_cnt == NULL || core_cnt == NULL){
		LOGERR("wrong parameter\n");
		return 1;
	}
	
	/* clear the output parameters */
	*socket_cnt = *node_cnt = *core_cnt = 0;
	*nodes = *cores = NULL;
	
	if(getenv(REEACT_USER_TOPOLOGY_CONFIG)) // use configuration file
		ret_val = reeact_get_topo_conf(nodes, cores, socket_cnt, 
					       node_cnt, core_cnt);
	if(ret_val) // err reading configuration file or no configuration file
		return reeact_get_topo_sysfs(nodes, cores, socket_cnt, node_cnt,
					     core_cnt);

	return 0;
}
