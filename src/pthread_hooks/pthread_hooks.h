/*
 * Header file for the REEact pthread hooking functions. These functions
 * replaces the original pthread functions at run-time.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __REEACT_PTHREAD_HOOKS_H__
#define __REEACT_PTHREAD_HOOKS_H__


/*
 * initialization function for the REEact pthread hooks.
 * Return value:
 *     0: success
 *     other: error, see the following defines
 */
#define REEACT_PTHREAD_HOOKS_ERR_LOAD_ORIGINAL_FUNCTION 1
int reeact_pthread_hooks_init();

#endif
