/*
 * Implementation of the conditional variable functions to replace pthread's
 * functions.
 * 
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <pthread.h>

#include "../policies/reeact_policy.h"


int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *cond_attr)
{
	return reeact_policy_pthread_cond_init((void*)cond, (void*)cond_attr);
}


int pthread_cond_signal(pthread_cond_t *cond)
{
	return reeact_policy_pthread_cond_signal((void*)cond);
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
	return reeact_policy_pthread_cond_broadcast((void*)cond);
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	return reeact_policy_pthread_cond_destroy((void*)cond);
}


int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	return reeact_policy_pthread_cond_wait((void*)cond, (void*)mutex);
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, 
			   const struct timespec *abstime)
{
	return reeact_policy_pthread_timedwait((void*)cond, (void*)mutex, 
					      (void*)abstime);
}
