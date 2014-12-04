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


typedef int (*pthread_mutex_init_type)(pthread_mutex_t *mutex, 
				       const pthread_mutexattr_t *mutexattr);

typedef int (*pthread_mutex_general_type)(pthread_mutex_t *mutex);

typedef int (*pthread_mutex_timedlock_type)(pthread_mutex_t *mutex, 
					    const struct timespec *abs_timeout);

typedef int (*pthread_cond_general_type)(pthread_cond_t *cond);
typedef int (*pthread_cond_init_type)(pthread_cond_t *cond, 
				     pthread_condattr_t *cond_attr);
typedef int (*pthread_cond_wait_type)(pthread_cond_t *cond, 
				      pthread_mutex_t *mutex);
typedef int (*pthread_cond_timedwait_type)(pthread_cond_t *cond, 
					   pthread_mutex_t *mutex, 
					   const struct timespec *abstime);


pthread_create_type real_pthread_create;
pthread_barrier_init_type real_pthread_barrier_init;
pthread_barrier_wait_type real_pthread_barrier_wait;
pthread_barrier_destroy_type real_pthread_barrier_destroy;

pthread_mutex_init_type real_pthread_mutex_init;
pthread_mutex_general_type real_pthread_mutex_lock;
pthread_mutex_general_type real_pthread_mutex_trylock;
pthread_mutex_timedlock_type real_pthread_mutex_timedlock;
pthread_mutex_general_type real_pthread_mutex_unlock;
pthread_mutex_general_type real_pthread_mutex_consistent;
pthread_mutex_general_type real_pthread_mutex_destroy;

pthread_cond_init_type real_pthread_cond_init;
pthread_cond_general_type real_pthread_cond_destroy;
pthread_cond_general_type real_pthread_cond_signal;
pthread_cond_general_type real_pthread_cond_broadcast;
pthread_cond_wait_type real_pthread_cond_wait;
pthread_cond_timedwait_type real_pthread_cond_timedwait;

/*
 * initialization function for REEact pthread hooks.
 */
int reeact_pthread_hooks_init(void *data)
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

	real_pthread_mutex_init = 
		(pthread_mutex_init_type)dlsym(RTLD_NEXT, "pthread_mutex_init");
	real_pthread_mutex_lock = 
		(pthread_mutex_general_type)dlsym(RTLD_NEXT, 
						  "pthread_mutex_lock");
	real_pthread_mutex_trylock = 
		(pthread_mutex_general_type)dlsym(RTLD_NEXT, 
						  "pthread_mutex_trylock");
	real_pthread_mutex_unlock = 
		(pthread_mutex_general_type)dlsym(RTLD_NEXT, 
						  "pthread_mutex_unlock");
	real_pthread_mutex_consistent = 
		(pthread_mutex_general_type)dlsym(RTLD_NEXT, 
						  "pthread_mutex_consistent");
	real_pthread_mutex_destroy = 
		(pthread_mutex_general_type)dlsym(RTLD_NEXT, 
						  "pthread_mutex_destroy");
	real_pthread_mutex_timedlock = 
		(pthread_mutex_timedlock_type)dlsym(RTLD_NEXT, 
						  "pthread_mutex_timedlock");
	
	real_pthread_cond_init = 
		(pthread_cond_init_type)dlsym(RTLD_NEXT, "pthread_cond_init");
	real_pthread_cond_destroy = 
		(pthread_cond_general_type)dlsym(RTLD_NEXT, 
						 "pthread_cond_destroy");
	real_pthread_cond_signal = 
		(pthread_cond_general_type)dlsym(RTLD_NEXT, 
						 "pthread_cond_signal");
	real_pthread_cond_broadcast = 
		(pthread_cond_general_type)dlsym(RTLD_NEXT, 
						 "pthread_cond_broadcast");
	real_pthread_cond_wait = 
		(pthread_cond_wait_type)dlsym(RTLD_NEXT, 
						 "pthread_cond_wait");
	real_pthread_cond_timedwait = 
		(pthread_cond_timedwait_type)dlsym(RTLD_NEXT, 
						 "pthread_cond_timedwait");


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

/*
 * cleanup function for REEact pthread hooks
 */
int reeact_pthread_hooks_cleanup(void *data)
{
	return 0;
}
