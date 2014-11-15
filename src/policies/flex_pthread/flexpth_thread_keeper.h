/*
 * Header file of the thread tracking/bookkeeping component for flex-pthread. 
 * This component keeps information about thread functions, threads and their 
 * core allocations.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __FLEX_PTHREAD_KEEPER_H__
#define __FLEX_PTHREAD_KEEPER_H__

#include "flexpth_common_defs.h"

/*
 * structure for holding thread information
 */
struct flexpth_thread_info{
	int tid; // thread id, -1 mean invalid thread entry
	int core_id; // core id of the core running this thread
	void *func; // the address of its thread function
	void *arg; // the application argument to thread function
	int tidx; // thread index assigned by flex-pthread
	int fidx; // thread function index assigned by flex-pthread
};

/*
 * structure for holding thread function information
 */
struct flexpth_thr_func_info{
	void *func; // the address of the thread function
	int thread_per_core[FLEX_PTHREAD_MAX_CORE_CNT]; // how many threads of
                                                        // this function assign-
                                                        // ed to each core
	int thread_cnt; // the total number of threads using this function
	int fidx; // 
};

/*
 * structure for holding all thread-related information
 */
struct flexpth_thread_keeper{
	void *threads; // a hash table that stores the thread information,
	               // i.e., struct flexpth_thread_info. The key is pthread
	               // id (pthread_t) and the hash table is the simple_hashx 
	               // from commontoolx
	int thread_cnt; // the total number of threads
	void *funcs; // a hash table with the thread function information,
	             // i.e., struct flexpth_thread_info. The key is the func-
	             // tion address, and the hash table is the simple_hashx 
                     // from commontoolx
	int func_cnt; // the total number of thread functions
};


/*
 * Initialized the thread keeper component
 * Input parameters:
 *     data: the REEact handle (struct reeact_data)
 * Return values:
 *     0: success
 *     1: data is NULL
 *     2: unable to allocate memory 
 */
int flexpth_thread_keeper_init(void *data);

/*
 * Clean up the thread keeper component
 * Input parameters:
 *     data: the REEact handle (struct reeact_data)
 * Return values:
 *     0: success
 *     1: data is NULL
 *     2: unable to free memory 
 */
int flexpth_thread_keeper_cleanup(void *data);

/*
 * Add the information of a new thread
 * Input parameters:
 *     data: the REEact handle (struct reeact_data)
 *     core_id: the id of the core to run thread
 *     func: the thread function address
 * Output parameters:
 *     tinfo: the newly created thread information
 * Return values:
 *     0: success
 *     1: wrong parameters
 *     2: error allocation memory
 *     3: error when saving thread information to hash table
 */
int flexpth_keeper_add_thread(void *data, int core_id, void* func,
			      struct flexpth_thread_info **tinfo);

/*
 * Remove the information of a thread
 * Input parameters:
 *     data: the REEact handle (struct reeact_data)
 *     tidx: thread index
 *     tinfo: thread information; if NULL tidx is used to locate the thread
 * Return values:
 *     0: success
 *     1: wrong parameters
 *     2: unable to find thread info
 *     3: unable to find thread function info
 */
int flexpth_keeper_remove_thread(void *data, int tidx, 
				 struct flexpth_thread_info *tinfo);


/*
 * Get the next function's information
 * Input parameters:
 *     data: the REEact handle (struct reeact_data)
 *     search_handle: an object for keeping track of currently enumerated 
 *                    functions; if NULL, then return the first function
 * Output parameters:
 *     search_handle: same as above, if returned handle is NULL, no more 
 *                    functions to enumerate
 *     finfo: the information of the object.
 * Return values:
 *     0: success
 *     1: wrong parameters
 */
int flexpth_keeper_get_next_func(void *data, void **search_handle, 
				 struct flexpth_thr_func_info **finfo);

/*
 * Change the running core of a thread
 * Input parameters:
 *     data: the REEact handle (struct reeact_data)
 *     core_id: the id of the core to run thread
 *     tidx: thread index
 * Return values:
 *     0: success
 *     1: wrong parameters
 *     2: unable to find thread info
 *     3: unable to find thread function info
 */
int flexpth_keeper_thread_migrate(void *data, int tidx, int core_id);

#endif
