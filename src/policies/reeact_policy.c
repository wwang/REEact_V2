/*
 * Skeleton implementation (and a dummy default policy) of user-specified REEact
 * management policy.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 *
 */

#include "../reeact.h"
#include "../utils/reeact_utils.h"
#include "../pthread_hooks/pthread_hooks_originals.h"

#ifdef _FLEX_PTHREAD_POLICY_
/*
 * for flex-pthread policy
 */
#include "./flex_pthread/flex_pthread.h"
#endif

/*
 * user policy initialization
 */

int reeact_policy_init(void *data)
{
#ifdef _REEACT_DEFAULT_POLICY_
	struct reeact_data *d = (struct reeact_data*)data;
	d->policy_data = NULL;

	return 0;
#elif _FLEX_PTHREAD_POLICY_
	return flexpth_init(data);
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
#elif _FLEX_PTHREAD_POLICY_
	return flexpth_cleanup(data);
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
#elif _FLEX_PTHREAD_POLICY_
	return flexpth_create_thread((pthread_t*)thread, (pthread_attr_t*)attr,
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
#elif _FLEX_PTHREAD_POLICY_
	return flexpth_barrier_init((pthread_barrier_t*)barrier,
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
#elif _FLEX_PTHREAD_POLICY_
	return flexpth_barrier_wait((pthread_barrier_t*)barrier);
#else
	// TODO: add user-policy here
	return 0;
#endif
}

int reeact_policy_pthread_barrier_destroy(void *barrier)
{
#ifdef _REEACT_DEFAULT_POLICY_
	return real_pthread_barrier_destroy((pthread_barrier_t*)barrier);
#elif _FLEX_PTHREAD_POLICY_
	return flexpth_barrier_destroy((pthread_barrier_t*)barrier);
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
