/*
 * Header file of the environment variable parsing component of flex-pthread.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __FLEX_PTHREAD_ENV_VAR_H__
#define __FLEX_PTHREAD_ENV_VAR_H__

/*
 * BEGIN: names of the environment variables
 */
/*
 * the list of cores to use, comma separated list, can have dashed ranges
 */
#define FLEXPTH_CORE_LIST_ENV "FLEXPTH_CORES"
/*
 * whether to control main thread:
 * 0 or unset: do not control main thread
 * 1: control main thread, and use worker thread's function address as
 *    main thread's function address
 * 2: same as above
 * other values: the function address of main thread, can be hex values
 */
#define FLEXPTH_MAIN_THR_HANDLING "FLEXPTH_MAIN_THR"
/*
 * OpenMP thread count variable
 */
#define OPENMP_THREAD_COUNT_ENV "OMP_NUM_THREADS"
/*
 * Enable GNU OpenMP load balancing variable
 */
#define FLEXPTH_OMP_LOAD_BALANCING_ENV "FLEXPTH_OMP_LOAD"

/*
 * END: names of the environment variables
 */


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
