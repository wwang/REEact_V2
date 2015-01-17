/*
 * Header file that defines the wrappers of the original gomp functions. Include
 * this file if the original functions are required.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __REEACT_GOMP_HOOKS_ORIGINALS_H__
#define __REEACT_GOMP_HOOKS_ORIGINALS_H__

/* #include "gomp_hooks.h" */

extern void (*real_GOMP_barrier)(void);


#endif
