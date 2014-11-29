/*
 * Implementation of the mutex functions that replace pthread mutex.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 *
 */

#include <pthread.h>

#include "../policies/reeact_policy.h"

int pthread_mutex_init(pthread_mutex_t *mutex, 
		       const pthread_mutexattr_t *attr)
{
	return reeact_policy_pthread_mutex_init((void*)mutex, (void*)attr);
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	return reeact_policy_pthread_mutex_lock((void*)mutex);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	return reeact_policy_pthread_mutex_trylock((void*)mutex);
}

int pthread_mutex_timedlock(pthread_mutex_t *mutex, 
			    const struct timespec *abs_timeout)
{
	return reeact_policy_pthread_mutex_timedlock((void*)mutex,
						     (void*)abs_timeout);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	return reeact_policy_pthread_mutex_unlock((void*)mutex);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	return reeact_policy_pthread_mutex_destroy((void*)mutex);
}

int pthread_mutex_consistent(pthread_mutex_t *mutex)
{
	return reeact_policy_pthread_mutex_consistent((void*)mutex);
}
