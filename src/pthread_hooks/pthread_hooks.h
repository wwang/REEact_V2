/*
 * Header file for the REEact pthread hooking functions. These functions
 * replaces the original pthread functions at run-time.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __REEACT_PTHREAD_HOOKS_H__
#define __REEACT_PTHREAD_HOOKS_H__


/*
 * Initialization function for REEact pthread hooks.
 *
 * Input parameters:
 *     data: the data used by the user policy. By default a pointer to the
 *           "struct reeact_data" is passed.
 * Return value:
 *     0: success
 *     other: error, see the following definitions
 */
#define REEACT_PTHREAD_HOOKS_ERR_LOAD_ORIGINAL_FUNCTION 1
int reeact_pthread_hooks_init(void *data);

/*
 * Cleanup function for REEact pthread hooks.
 *
 * Input parameters:
 *     data: the data used by the user policy. By default a pointer to the
 *           "struct reeact_data" is passed.
 * Return value:
 *     0: success
 *     other: error, see the following definitions   
 */
int reeact_pthread_hooks_cleanup(void *data);

#endif
