/*
 * Header file of the environment variable parsing component of flex-pthread.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __FLEX_PTHREAD_ENV_VAR_H__
#define __FLEX_PTHREAD_ENV_VAR_H__

/*
 * names of the environment variables
 */
#define FLEXPTH_CORE_LIST_ENV "FLEXPTH_CORES"


/*
 * structure for storing core list
 */ 
struct flexpth_core_list{
	int *cores;
	int core_cnt;
};
/*
 * Read and parse the environment variables
 *
 * Input parameters:
 *     data: the REEact handle (struct reeeact_data)
 * Return value:
 *     0: success
 *     1: data is NULL
 */
int flexpth_parse_env_vars(void *data);

/*
 * Cleanup the environment variable component
 * 
 * Input parameters:
 *     data: the REEact handle (struct reeeact_data)
 * Return value:
 *     0: success
 */
int flexpth_env_vars_cleanup(void *data);


#endif
