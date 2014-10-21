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
pthread_create_type real_pthread_create;

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
	real_pthread_create = (pthread_create_type)dlsym(RTLD_NEXT, 
							 "pthread_create");

	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "Error opening pthread_create with error :%s\n",
			error);
		ret_val = REEACT_PTHREAD_HOOKS_ERR_LOAD_ORIGINAL_FUNCTION;
		goto error;
	}
	
	return 0;

 error:
	return ret_val;
}
