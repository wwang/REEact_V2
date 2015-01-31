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
 *     abs_timeout: by default a "struct timespec*) type
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


/*
 * pthread conditional variable hooks of the user policy. 
 * 
 * Input parameters (see the pthread_cond manuals for more info):
 *     mutex: by default a "pthread_cond_t*" type
 *     attr: by default a "pthread_condattr_t*" type
 *     abstime: by default a "struct timespec*) type
 * Return values:
 *     same as corresponding pthread_mutex functions or by user definition
 */
int reeact_policy_pthread_cond_init(void *cond, void *attr);
int reeact_policy_pthread_cond_signal(void *cond);
int reeact_policy_pthread_cond_broadcast(void *cond);
int reeact_policy_pthread_cond_destroy(void *cond);
int reeact_policy_pthread_cond_wait(void *cond, void *mutex);
int reeact_policy_pthread_cond_timedwait(void *cond, void *mutex, void *abstime);


/*
 * gomp barrier functions
 */
//void reeact_policy_GOMP_barrier();
void reeact_gomp_barrier_init(void *, unsigned);
void reeact_gomp_barrier_reinit(void *, unsigned);
void reeact_gomp_barrier_destroy(void *);
void reeact_gomp_barrier_wait(void *);
void reeact_gomp_barrier_wait_last(void *);
void reeact_gomp_barrier_wait_end(void *, unsigned int);
void reeact_gomp_team_barrier_wait(void *);
void reeact_gomp_team_barrier_wait_end(void *, unsigned int);
void reeact_gomp_team_barrier_wake(void *, int);
void reeact_gomp_team_barrier_set_task_pending(void *);
void reeact_gomp_team_barrier_clear_task_pending(void *);
void reeact_gomp_team_barrier_set_waiting_for_tasks(void *);
void reeact_gomp_team_barrier_done(void *, unsigned int);
int reeact_gomp_team_barrier_waiting_for_tasks(void *);
int reeact_gomp_barrier_last_thread(unsigned int);
unsigned int reeact_gomp_barrier_wait_start(void *);

#endif
