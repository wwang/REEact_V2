/*
 * Implementation of the barrier functions that replace pthread barrier.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <pthread.h>

#include "../policies/reeact_policy.h"

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
	return reeact_policy_pthread_barrier_wait((void*)barrier);	
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
	return reeact_policy_pthread_barrier_destroy((void*)barrier);
}
int pthread_barrier_init(pthread_barrier_t *barrier,
			 const pthread_barrierattr_t *attr, 
			 unsigned count)
{
	return reeact_policy_pthread_barrier_init((void*)barrier, (void*)attr, 
						  count);
}

