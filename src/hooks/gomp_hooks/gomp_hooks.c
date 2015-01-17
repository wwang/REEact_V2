/*
 * Initialization and cleanup routines for REEact gomp hooks. These routines are
 * responsible for:
 *     1. opening and closing the original GOMP library
 *     2. providing the original GOMP functions
 *     3. maintaining internal data structures
 *
 * Author: Wei Wang <wwang@virginia.edu?
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>

#include "gomp_hooks.h"

/*
 * real gomp functions
 */
typedef void (*GOMP_barrier_type)(void);


GOMP_barrier_type real_GOMP_barrier;

/*
 * initialization function for REEact gomp hooks
 */
int reeact_gomp_hooks_init(void *data)
{
	char *error = NULL;
	int ret_val = 0;

	/*
	 * locate the original gomp functions
	 */
	dlerror();    // Clear any existing error
	real_GOMP_barrier = (GOMP_barrier_type)dlsym(RTLD_NEXT, "GOMP_barrier");

	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "Error opening original gomp functions with "
			"error :%s\n", error);
		ret_val = REEACT_GOMP_HOOKS_ERR_LOAD_ORIGINAL_FUNCTION;
		goto error;
	}
	
	return 0;

 error:
	return ret_val;
}


/*
 * cleanup function for REEact pthread hooks
 */
int reeact_gomp_hooks_cleanup(void *data)
{
	return 0;
}
