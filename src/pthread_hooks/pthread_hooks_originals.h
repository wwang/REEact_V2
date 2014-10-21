/*
 * HEADER file that defines wrappers of the original pthread functions. Include
 * this file if there is a need to call the real pthread functions.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */


#ifndef __REEACT_PTHREAD_HOOKS_ORIGINALS_H__
#define __REEACT_PTHREAD_HOOKS_ORIGINALS_H__

#include <pthread.h>

extern int (*real_pthread_create)(pthread_t *thread, const pthread_attr_t *attr,
				  void *(*start_routine) (void *), void *arg);

#endif

