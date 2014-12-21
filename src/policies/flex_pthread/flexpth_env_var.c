/*
 * Implementation of the environment variable parsing component of flex-pthread.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdio.h>
#include <stdlib.h>

#include <common_toolx.h>

#include "../../reeact.h"
#include "../../utils/reeact_utils.h"

#include "flex_pthread.h"
#include "flexpth_env_var.h"

/*
 * parsing the core list
 * Input parameters
 *     rh: REEact data
 *
 * Return values:
 *     0: success
 *     1: policy data is NULL
 *     2: unable to allocate space
 *     3: parsing core list error
 */
int flexpth_parse_core_list(struct reeact_data *rh)
{
	char *env;
	struct flexpth_core_list *cl;
	int ret_val = 0;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;

	if(fh == NULL){
		LOGERR("flex-pthread data is NULL\n");
		return 1;
	}

	/*
	 * allocate space to store core list
	 */
	cl = (struct flexpth_core_list*)
		malloc(sizeof(struct flexpth_core_list));
	if(cl == NULL){
		LOGERRX("Unable to allocate space for core list: ");
		return 2;
	}
	fh->core_list = (void*)cl;

	/*
	 * read core list from environment variable
	 */
	env = getenv(FLEXPTH_CORE_LIST_ENV);
	DPRINTF("Parsing core list: %s\n", env);
	if(env != NULL)
		ret_val = parse_value_list_expand(env, (void**)&cl->cores, 
					   &(cl->core_cnt), 0);
	if(ret_val != 0){
		LOGERR("Unable to parse core list \"%s\", with error %d\n", env,
		       ret_val);
		ret_val = 3;
	}
	else 
		ret_val = 0;
	/*
	 * used all cores
	 */
	if(env == NULL || ret_val == 3 || cl->core_cnt == 0){
		int i;
		cl->core_cnt = rh->topology.core_cnt * rh->topology.node_cnt *
			rh->topology.socket_cnt;
		DPRINTF("Using all %d cores\n", cl->core_cnt);
		cl->cores = malloc(sizeof(int) * cl->core_cnt);
		if(cl->cores == NULL){
			LOGERRX("unable to allocate space for cores array: ");
			return 2;
		}
		for(i = 0; i< cl->core_cnt; i++)
			cl->cores[i] = i;
	}
#ifdef _REEACT_DEBUG_
	{
		int i;
		fprintf(stderr, "cores to use: ");
		for(i = 0; i< cl->core_cnt; i++)
			fprintf(stderr, "%d,", cl->cores[i]);
		fprintf(stderr, "\n");
	}
#endif

	return 0;
}


/*
 * parsing the environment variable that controls the handling of main thread
 * Input parameters
 *     rh: REEact data
 *
 * Return values:
 *     0: success
 */
int flexpth_parse_main_thread_handling(struct reeact_data *rh)
{
	char *env;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;

	fh->control_main_thr = 0;
	env = getenv(FLEXPTH_MAIN_THR_HANDLING);

	if(env != NULL)
		/* environment variable not set */
		fh->control_main_thr = strtoull(env, NULL, 0);
	if(fh->control_main_thr == 2) 
		/* 2 is treated the same as 1 */
		fh->control_main_thr = 1;

	DPRINTF("Main thread control flag: 0x%08x\n", fh->control_main_thr); 

	return 0;
}


/*
 * Read and parse the environment variables
 */
int flexpth_parse_env_vars(void *data)
{

	if(data == NULL){
		LOGERR("REEact data is NULL\n");
		return 1;
	}

	flexpth_parse_core_list((struct reeact_data*)data);
	flexpth_parse_main_thread_handling((struct reeact_data*)data);

	return 0;
}


/*
 * Cleanup the environment variable component
 */
int flexpth_env_vars_cleanup(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	
	if(rh == NULL || fh == NULL){
		LOGERR("REEact data (%p) or flex-pthread data (%p) is NULL\n",
		       rh, fh);
		return 1;
	}

	if(fh->core_list != NULL){
		struct flexpth_core_list *cl = 
			(struct flexpth_core_list*)fh->core_list;
		if(cl->cores != NULL)
			free(cl->cores);
		free(cl);
	}
	
	
	return 0;
}
