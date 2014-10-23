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

