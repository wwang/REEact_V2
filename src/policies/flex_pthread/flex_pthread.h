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
#include <time.h>

struct flexpth_data{
	void *thread_keeper; // a handle to thread keeper component, also
                             // a pointer to struct flexpth_thread_keeper
	void *core_list; // the list of cores to use.
	void *barriers; // pointer to all barriers struct flexpth_all_tree_bars
	/*
	 * flags controls whether to control main thread; 
	 * if 0, main thread is not controlled; 
	 * if 1, main thread is treated as using the thread functions of the 
	 *       first created worker thread; but this thread function address
	 *       is undetermined yet;
	 * if 2, like 1, except the thread function address is determined;
	 * all other values represents the true main thread function address
	 */
	unsigned long long control_main_thr;  
	int omp_thr_cnt; // the number of threads for OpenMP
	int enable_omp_load_balancing; //  enable OpenMP load balancing or not
	/*
	 * core list for OpenMP threads, used when OpenMP load balancing is
	 * enabled
	 */
	void *omp_core_list; 
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
 * Return values: for compatibility issue, same as pthread_create
 *     0: success
 *     
 * 
 */
int flexpth_create_thread(pthread_t *thread, pthread_attr_t *attr,
			  void *(*start_routine)(void *), void *arg);

/*
 * Add the main thread under-control
 * Input parameter:
 *     data: a pointer to REEact data (struct reeact_data *)
 * Return value:
 *     0: success
 *     1: data is NULL
 *     2: unable to set core affinity
 *     3: unable adding main thread to thread keeper
 */
int flexpth_control_main_thr(void *data);


/*
 * Barrier synchronization functions for flex-pthread. 
 * 
 * Input parameters:
 *      barrier: the barrier to wait at
 *      attr: attribute of the barrier
 *      count: the number of threads that use this barrier
 * Return values:
 *      0: success
 *      other: the same as pthread standards
 */
int flexpth_barrier_wait(pthread_barrier_t *barrier);
int flexpth_barrier_destroy(pthread_barrier_t *barrier);
int flexpth_barrier_init(pthread_barrier_t *barrier, 
			 const pthread_barrierattr_t *attr, unsigned count);

/*
 * Mutex synchronization functions for flex-pthread
 * Input parameters:
 *      barrier: the barrier to wait at
 *      attr: attribute of the barrier
 * Return values:
 *      0: success
 *      other: the same as pthread standards
 */
int flexpth_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int flexpth_mutex_lock(pthread_mutex_t *mutex);
int flexpth_mutex_unlock(pthread_mutex_t *mutex);
int flexpth_mutex_trylock(pthread_mutex_t *mutex);
int flexpth_mutex_timedlock(pthread_mutex_t *mutex, 
			   const struct timespec *abs_timeout);
int flexpth_mutex_destroy(pthread_mutex_t *mutex);
int flexpth_mutex_consistent(pthread_mutex_t *mutex);

/*
 * Conditional variable synchronization functions for flex-pthread
 * Input parameters:
 *      cond: the conditional variable to wait at
 *      attr: attribute of the barrier
 *      mutex: the mutex associated with the conditional variable
 * Return values:
 *      0: success
 *      other: the same as pthread standards
 */
int flexpth_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int flexpth_cond_signal(pthread_cond_t *cond);
int flexpth_cond_broadcast(pthread_cond_t *cond);
int flexpth_cond_destroy(pthread_cond_t *cond);
int flexpth_cond_wait(pthread_cond_t *cond, pthread_mutex_t *m);
int flexpth_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *m, 
			   const struct timespec *abs_timeout);

/*
 * GOMP barrier functions
 */
void flexpth_GOMP_barrier();
int flexpth_gomp_barrier_init(void *barrier, unsigned count);
int flexpth_gomp_barrier_reinit(void *barrier, unsigned count);
int flexpth_gomp_barrier_destroy(void *barrier);
int flexpth_gomp_barrier_wait(void *barrier);
int flexpth_gomp_team_barrier_wait(void *barrier);
int flexpth_gomp_barrier_wait_start(void *bar, unsigned int *ret_val);
int flexpth_gomp_barrier_wait_end(void *bar, unsigned int state);
int flexpth_gomp_team_barrier_wait_end(void *bar, unsigned int state);
int flexpth_gomp_barrier_last_thread(unsigned int state, int *ret_val);
int flexpth_gomp_barrier_wait_last(void *bar);
int flexpth_gomp_team_barrier_wake(void *bar, int count);
int flexpth_gomp_team_barrier_set_task_pending(void *bar);
int flexpth_gomp_team_barrier_clear_task_pending(void *bar);
int flexpth_gomp_team_barrier_set_waiting_for_tasks(void *bar);
int flexpth_gomp_team_barrier_waiting_for_tasks(void *bar, int *ret_val);
int flexpth_gomp_team_barrier_done(void *bar, unsigned int state);

#endif 
