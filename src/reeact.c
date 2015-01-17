/*
 * This file contains the implementation of the constructor and destructor 
 * functions for the REEact library
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "reeact.h"
#include "./utils/reeact_utils.h"
#include "./pthread_hooks/pthread_hooks.h"
#include "./hooks/gomp_hooks/gomp_hooks.h"
#include "./policies/reeact_policy.h"

struct reeact_data *reeact_handle = NULL;

/*
 * REEact initialization procedure for each process
 *
 * Input parameters:
 *     rh: the pointer to the struct reeact_data of this process
 * Return values:
 *     0: success
 *     1: rh is NULL
 */
int reeact_per_proc_init(struct reeact_data *rh)
{
	int pid;

	pid = getpid();
	
	if(rh == NULL){
		fprintf(stderr, "%d %s: reeact_per_proc_init: "
			"REEact data is NULL\n", pid, 
			program_invocation_short_name);
		return 1;
	}

	rh->pid = pid;
	rh->proc_name_short = program_invocation_short_name;
	rh->proc_name_long = program_invocation_name;

	return 0;
}

/*
 * Topology logging
 */
inline void reeact_log_topology(struct reeact_data *rh)
{
#ifdef _REEACT_DEBUG_
	int i,j,k;
	int node_id;
	int *nodes, *cores;
	int socket_cnt, node_cnt, core_cnt;
	/*
	 * Debug logging for topology detection
	 */
	DPRINTF("Socket count is %d, node per socket is %d, cores per node is "
		"%d\n", rh->topology.socket_cnt, 
		rh->topology.node_cnt, 
		rh->topology.core_cnt);
	
	socket_cnt = rh->topology.socket_cnt;
	node_cnt = rh->topology.node_cnt;
	core_cnt = rh->topology.core_cnt;
	nodes = rh->topology.nodes;
	cores = rh->topology.cores;
	for(i = 0; i < socket_cnt; i++){
		fprintf(stderr, "Socket %d:\n", i);
		for(j = 0; j < node_cnt; j++){
			node_id = nodes[i*node_cnt+j];
			fprintf(stderr, "\t Node %d:\n\t\t", node_id);
			for(k = 0; k < core_cnt; k++){
				fprintf(stderr, "%d ", 
					cores[node_id * core_cnt + k]);
			}
			fprintf(stderr, "\n");	
		}
	}
	
	return;
#endif
}

/*
 * initialization function for REEact
 */
__attribute__ ((__constructor__)) 
void reeact_init(void) 
{
	int ret_val = 0;
	
        DPRINTF("reeact initialization\n");

	// allocate data for reeact
	reeact_handle = 
		(struct reeact_data*)calloc(1, sizeof(struct reeact_data));
	if(reeact_handle == NULL){
		LOGERRX("Unable to allocate memory for REEact data: ");
		return;
	}

	// per process initialization
	reeact_per_proc_init(reeact_handle);
	// determine processor topology
	reeact_get_topology(&(reeact_handle->topology.nodes), 
			    &(reeact_handle->topology.cores),
			    &(reeact_handle->topology.socket_cnt),
			    &(reeact_handle->topology.node_cnt),
			    &(reeact_handle->topology.core_cnt));
	reeact_log_topology(reeact_handle);
		
	// pthread hooks initialization
	ret_val = reeact_pthread_hooks_init((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error initializing pthread hooks with error %d\n",
		       ret_val);

	// gomp hooks initialization
	ret_val = reeact_gomp_hooks_init((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error initializing gomp hooks with error %d\n",
		       ret_val);
	
	// user policy initialization
	ret_val = reeact_policy_init((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error initializing user policy with error %d\n",
		       ret_val);

	return;
}

/*
 * cleanup function for REEact
 */
__attribute__ ((__destructor__)) 
void reeact_cleanup(void) 
{
	int ret_val;

        DPRINTF("reeact cleanup\n");

	ret_val = reeact_policy_cleanup((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error cleaning up pthread hooks with error %d\n",
		       ret_val);

	ret_val = reeact_pthread_hooks_cleanup((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error cleaning up user policy with error %d\n",
		       ret_val);

	if(reeact_handle != NULL)
		free(reeact_handle);

	return;
}
