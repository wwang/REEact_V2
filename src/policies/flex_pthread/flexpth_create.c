/*
 * Implementation of thread creation function for flex-pthread
 *
 * Author: Wei Wang <wwang@virginia.edu>
 *
 */

#include <pthread.h>

#include "../../pthread_hooks/pthread_hooks_originals.h"

int flexpth_create_thread(pthread_t *thread, pthread_attr_t *attr,
			  void *(*start_routine)(void *), void *arg)
{
	return real_pthread_create(thread, attr, start_routine, arg);
}
