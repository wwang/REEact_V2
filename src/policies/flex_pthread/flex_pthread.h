/*
 * Declarations of flex-pthread data types and functions. Only exported 
 * functions are declared here.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 * 
 */

#ifndef __FLEX_PTHREAD_H_
#define __FLEX_PTHREAD_H_

#include <pthread.h>

struct flexpth_data{
	int dummy;
};

/*
 * Initialize flex-pthread policy
 *
 * Input parameters:
 *     data: a pointer to REEact data (struct reeact_data *, actually)
 * Return values:
 *     0: success
 *     1: data is NULL, wrong parameter
 *     2: unable to allocate memory for flex-pthread data
 */
int flexpth_init(void *data);

/*
 * Cleanup function for flex-pthread policy
 *
 * Input parameters:
 *     data: a pointer to REEact data (struct reeact_data *, actually)
 * Return values:
 *     0: success
 *     1: data is NULL, wrong parameter
 *     2: flex-pthread data is NULL
 */
int flexpth_cleanup(void *data);

/*
 * Create a new thread.
 *
 * Input parameters:
 *     thread, attr, start_routine, arg: see the manual of pthread_create
 *
 * Return values:
 *     0: success
 * 
 */
int flexpth_create_thread(pthread_t *thread, pthread_attr_t *attr,
			  void *(*start_routine)(void *), void *arg);


/*
 * Barrier synchronization functions for flex-pthread. 
 * 
 * Input parameters:
 *      barrier: the barrier to wait at
 *      attr: attribute of the barrier
 *      count: the number of threads that use this barrier
 * Return values:
 *      0: success
 */
int flexpth_barrier_wait(pthread_barrier_t *barrier);
int flexpth_barrier_destroy(pthread_barrier_t *barrier);
int flexpth_barrier_init(pthread_barrier_t *barrier, 
			 const pthread_barrierattr_t *attr, unsigned count);



#endif 
