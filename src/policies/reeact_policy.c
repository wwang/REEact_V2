/*
 * Skeleton implementation (and a dummy default policy) of user-specified REEact
 * management policy.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>

#include "../reeact.h"
#include "../utils/reeact_utils.h"
#include "../pthread_hooks/pthread_hooks_originals.h"
#include "../hooks/gomp_hooks/gomp_hooks.h"
#include "../hooks/gomp_hooks/gomp_hooks_originals.h"

/*
 * user policy initialization
 */

int reeact_policy_init(void *data)
{
#ifdef _REEACT_DEFAULT_POLICY
	struct reeact_data *d = (struct reeact_data*)data;
	d->policy_data = NULL;

	return 0;
#else
	// TODO: add user-policy here
	return 0;
#endif
}

/*
 * user policy cleanup
 */
int reeact_policy_cleanup(void *data)
{
#ifdef _REEACT_DEFAULT_POLICY
	return 0;
#else
	// TODO: add user-policy here
	return 0;
#endif
}

/* 
 * pthread_create hook 
 */
int reeact_policy_pthread_create(void *thread, void *attr, 
				void *(*start_routine) (void *), void *arg)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_create((pthread_t*)thread, (pthread_attr_t*)attr,
				   start_routine, arg);
#else
	// TODO: add user-policy here
	return 0;
#endif
}

/* 
 * pthread_barrier hooks 
 */
int reeact_policy_pthread_barrier_init(void *barrier, void *attr, 
				       unsigned count)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_barrier_init((pthread_barrier_t*)barrier,
					 (pthread_barrierattr_t*)attr,
					 count);
#else
	// TODO: add user-policy here
	return 0;
#endif
}

int reeact_policy_pthread_barrier_wait(void *barrier)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_barrier_wait((pthread_barrier_t*)barrier);
#else
	// TODO: add user-policy here
	return 0;
#endif
}

int reeact_policy_pthread_barrier_destroy(void *barrier)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_barrier_destroy((pthread_barrier_t*)barrier);
#else
	// TODO: add user-policy here
	return 0;
#endif
}

/*
 * pthread_mutex hooks
 */

int reeact_policy_pthread_mutex_init(void *mutex, void *attr)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_mutex_init((pthread_mutex_t*)mutex,
				       (pthread_mutexattr_t*)attr);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_mutex_lock(void *mutex)
{
#ifdef _REEACT_DEFAULT_POLICY_
	if(real_pthread_mutex_lock == NULL)
		real_pthread_mutex_lock = dlsym(RTLD_NEXT, 
						"pthread_mutex_lock");
	return real_pthread_mutex_lock((pthread_mutex_t*)mutex);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_mutex_trylock(void *mutex)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_mutex_trylock((pthread_mutex_t*)mutex);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_mutex_timedlock(void *mutex, void *abs_timeout)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_mutex_timedlock((pthread_mutex_t*)mutex,
					    (struct timespec*)abs_timeout);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_mutex_unlock(void *mutex)
{
#ifdef _REEACT_DEFAULT_POLICY_
	if(real_pthread_mutex_unlock == NULL)
		real_pthread_mutex_unlock = dlsym(RTLD_NEXT, 
						  "pthread_mutex_unlock");
	return real_pthread_mutex_unlock((pthread_mutex_t*)mutex);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_mutex_consistent(void *mutex)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_mutex_consistent((pthread_mutex_t*)mutex);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_mutex_destroy(void *mutex)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_mutex_destroy((pthread_mutex_t*)mutex);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_cond_init(void *cond, void *attr)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_cond_init((pthread_cond_t*)cond, 
				      (pthread_condattr_t*)attr);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_cond_signal(void *cond)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_cond_signal((pthread_cond_t*)cond);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_cond_broadcast(void *cond)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_cond_broadcast((pthread_cond_t*)cond);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_cond_destroy(void *cond)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_cond_destroy((pthread_cond_t*)cond);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_cond_wait(void *cond, void *mutex)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_cond_wait((pthread_cond_t*)cond,
				      (pthread_mutex_t*)mutex);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_policy_pthread_cond_timedwait(void *cond, void *mutex, void *abstime)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_cond_timedwait((pthread_cond_t*)cond,
					   (pthread_mutex_t*)mutex,
					   (struct timespec*)abstime);
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

/* void reeact_policy_GOMP_barrier() */
/* { */
/* #ifdef _REEACT_DEFAULT_POLICY_ */
/* 	return real_GOMP_barrier(); */
/* #else */
/* 	// TODO: add user-policy here */
/* 	return; */
/* #endif	 */
/* } */
int reeact_gomp_barrier_init(void *bar, unsigned count)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p, count %u\n", __FUNCTION__, bar, count);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_barrier_reinit(void *bar, unsigned count)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p, count %u\n", __FUNCTION__, bar, count);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_barrier_destroy(void *bar)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_barrier_wait(void *bar)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_barrier_wait_last(void *bar)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_barrier_wait_end(void *bar, unsigned int state)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p and state %d\n", __FUNCTION__, bar, 
		state);	
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_team_barrier_wait(void *bar)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_team_barrier_wait_end(void *bar, unsigned int state)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p and state %d\n", __FUNCTION__, bar, 
		state);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_team_barrier_wake(void *bar, int count)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p and count %d\n", __FUNCTION__, bar, count);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_team_barrier_set_task_pending(void *bar)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_team_barrier_clear_task_pending(void *bar)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_team_barrier_set_waiting_for_tasks(void *bar)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return;
#endif	
}

int reeact_gomp_team_barrier_done(void *bar, unsigned int state)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p and state %d\n", __FUNCTION__, bar, 
		state);
	return 1;
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_gomp_team_barrier_waiting_for_tasks(void *bar, int *ret_val)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_gomp_barrier_last_thread(unsigned int state, int *ret_val)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with state %d\n", __FUNCTION__, state);
	return 1;
#else
	// TODO: add user-policy here
	return 0;
#endif	
}

int reeact_gomp_barrier_wait_start(void *bar, unsigned int *ret_val)
{
#ifdef _REEACT_DEFAULT_POLICY_
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return 1;
#else
	// TODO: add user-policy here
	return 0;
#endif		
}
