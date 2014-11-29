/*
 * Header file for the user-specified REEact management policies. This file 
 * contains the declaration of functions that are used by other modules of
 * REEact, such as user-policy initialization function and user-defined 
 * pthread hooks. Feel free to add your own functions.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __REEACT_USER_POLICY_H__
#define __REEACT_USER_POLICY_H__

/*
 * User policy initialization function. This function is called as part of 
 * REEact initialization process. Note that this function may be called before 
 * the "main" function of the program is executed.
 *
 * Input parameters:
 *     data: the data used by the user policy. By default a pointer to the
 *           "struct reeact_data" is passed.
 * Return values:
 *     0: success
 *     other: by user definition
 */
int reeact_policy_init(void *data);

/* 
 * User policy cleanup function. This function is called when REEact is unloaded
 * form a program.
 * 
 * Input parameters:
 *     data: the data used by user policy. By default a pointer to the "struct
 *     reeact_data" is passed.

 * Return values:
 *     0: success
 *     other: by user definition
 */
int reeact_policy_cleanup(void *data);

/*
 * pthread_create hook of the user policy.
 *
 * Input parameters (see the pthread_create manual for more info):
 *     thread: by default a "pthread_t*" type
 *     attr: by default a "pthread_attr_t*" type
 *     start_routine: the thread entry point function
 *     arg: parameter passed to the "start_routine"
 * Return values:
 *     same as pthread_create or by user definition
 */
int reeact_policy_pthread_create(void *thread, void *attr, 
				 void *(*start_routine) (void *), void *arg);

/*
 * pthread_barrier hooks of the user policy.
 *
 * Input parameters (see the pthread_barrier manuals for more info):
 *     barrier: by default a "pthread_barrier_t*" type
 *     attr: by default a "pthread_barrierattr_t*" type
 *     count: the number of threads that use this barrier
 * Return values:
 *     same as corresponding pthread_barrier functions or by user definition
 */
int reeact_policy_pthread_barrier_init(void *barrier, void *attr, 
				       unsigned count);
int reeact_policy_pthread_barrier_wait(void *barrier);
int reeact_policy_pthread_barrier_destroy(void *barrier);

/*
 * pthread mutex hooks of the user policy.
 * 
 * Input parameters (see the pthread_mutex manuals for more info):
 *     mutex: by default a "pthread_mutex_t*" type
 *     attr: by default a "pthread_mutexattr_t*" type
 * Return values:
 *     same as corresponding pthread_mutex functions or by user definition
 */
int reeact_policy_pthread_mutex_init(void *mutex, void *attr);
int reeact_policy_pthread_mutex_lock(void *mutex);
int reeact_policy_pthread_mutex_trylock(void *mutex);
int reeact_policy_pthread_mutex_timedlock(void *mutex, void *abs_timeout);
int reeact_policy_pthread_mutex_unlock(void *mutex);
int reeact_policy_pthread_mutex_consistent(void *mutex);
int reeact_policy_pthread_mutex_destroy(void *mutex);

#endif
