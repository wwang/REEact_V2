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
#include "gomp_hooks_originals.h"
#include ".././../utils/reeact_utils.h"

/*
 * real gomp function types
 */
typedef void (*GOMP_barrier_type)(void);

/*
 * real gomp functions
 */ 
GOMP_barrier_type real_GOMP_barrier;


/*
 * initialization function for REEact gomp hooks
 */
int reeact_gomp_hooks_init(void *data)
{
	return 0;
}


/*
 * cleanup function for REEact pthread hooks
 */
int reeact_gomp_hooks_cleanup(void *data)
{
	return 0;
}
