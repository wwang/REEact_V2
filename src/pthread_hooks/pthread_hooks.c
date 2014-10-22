/*
 * Initialization and cleanup routines for REEact pthread hooks. These routines 
 * are responsible for:
 *     1. opening and closing the original pthread library
 *     2. providing the original pthread functions
 *     3. maintaining internal data structures
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>

#include "pthread_hooks.h"

/* 
 * real pthread functions 
 */
typedef int (*pthread_create_type)(pthread_t *thread, 
				   const pthread_attr_t *attr,
				   void *(*start_routine) (void *), void *arg);
typedef int (*pthread_barrier_init_type)(pthread_barrier_t *barrier, 
					 const pthread_barrierattr_t *attr,
					 unsigned count);
typedef int (*pthread_barrier_wait_type)(pthread_barrier_t *barrier);
typedef int (*pthread_barrier_destroy_type)(pthread_barrier_t *barrier);

pthread_create_type real_pthread_create;
pthread_barrier_init_type real_pthread_barrier_init;
pthread_barrier_wait_type real_pthread_barrier_wait;
pthread_barrier_destroy_type real_pthread_barrier_destroy;

/*
 * initialization function for the REEact pthread hooks.
 */
int reeact_pthread_hooks_init()
{
	char *error = NULL;
	int ret_val = 0;

	/*
	 * locate the original pthread functions
	 */
	dlerror();    // Clear any existing error
	real_pthread_create = 
		(pthread_create_type)dlsym(RTLD_NEXT, "pthread_create");
	real_pthread_barrier_init = 
		(pthread_barrier_init_type)dlsym(RTLD_NEXT, 
						 "pthread_barrier_init");
	real_pthread_barrier_wait = 
		(pthread_barrier_wait_type)dlsym(RTLD_NEXT, 
						 "pthread_barrier_wait");
	real_pthread_barrier_destroy = 
		(pthread_barrier_destroy_type)dlsym(RTLD_NEXT, 
						    "pthread_barrier_destroy");

	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "Error opening original pthread functions with "
			"error :%s\n", error);
		ret_val = REEACT_PTHREAD_HOOKS_ERR_LOAD_ORIGINAL_FUNCTION;
		goto error;
	}
	
	return 0;

 error:
	return ret_val;
}
