/*
 * The implementation of the REEact pthread hook for pthread_create.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */


#include <stdio.h>
#include <pthread.h>


#include "../utils/reeact_utils.h"
#include "../policies/reeact_policy.h"

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, 
		   void *(*start_routine) (void *), void *arg)
{
	DPRINTF("pthread_create called\n");

	return reeact_policy_pthread_create((void*)thread, (void*)attr, 
					    start_routine, arg);
}
