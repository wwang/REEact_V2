/*
 * Header file for the REEact GNU OpenMP hooking functions.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __REEACT_GNU_OPENMP_HOOKS_H__
#define __REEACT_GNU_OPENMP_HOOKS_H__


/*
 * gomp barrier structure definition, directly copied from GCC source code
 */
typedef struct
{
	/* Make sure total/generation is in a mostly read cacheline, while
	   awaited in a separate cacheline.  */
	unsigned total __attribute__((aligned (64)));
	unsigned generation;
	unsigned awaited __attribute__((aligned (64)));
} gomp_barrier_t;
typedef unsigned int gomp_barrier_state_t;

/*
 * Initialization function for REEact gomp hooks.
 *
 * Input parameters:
 *     data: the data used by the user policy. By default a pointer to the
 *           "struct reeact_data" is passed.
 * Return value:
 *     0: success
 *     other: error, see the following definitions
 */
#define REEACT_GOMP_HOOKS_ERR_LOAD_ORIGINAL_FUNCTION 1
int reeact_gomp_hooks_init(void *data);

/*
 * Cleanup function for REEact gomp hooks.
 *
 * Input parameters:
 *     data: the data used by the user policy. By default a pointer to the
 *           "struct reeact_data" is passed.
 * Return value:
 *     0: success
 *     other: error, see the following definitions   
 */
int reeact_gomp_hooks_cleanup(void *data);


#endif
