/*
 * Implementation of the barrier functions that replace pthread barrier.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */


#include <stdlib.h>

#include "../utils/reeact_utils.h"
#include "../fastsync/fastsync.h"

#include "pthread_hooks_originals.h"

int pthread_barrier_wait(pthread_barrier_t *barrier)
{
	fastsync_barrier *bar = *(fastsync_barrier**) barrier;

	return fastsync_barrier_wait(bar);
}

int pthread_barrier_destroy(pthread_barrier_t *barrier)
{
	int ret_val;

	fastsync_barrier *bar = *(fastsync_barrier**) barrier;
	
	ret_val = fastsync_barrier_destroy(bar);
	
	if(bar != NULL)
		free(bar);
	
	return ret_val;
}
int pthread_barrier_init(pthread_barrier_t *barrier,
			 const pthread_barrierattr_t *attr, 
			 unsigned count)
{
	/* 
	 * create a new fast sync barrier object and save it in the
	 * pthread_barrier_t *barrier
	 */
	fastsync_barrier *bar = malloc(sizeof(fastsync_barrier));
	*((fastsync_barrier**)barrier) = bar;
	
	// initialize fast barrier
	return fastsync_barrier_init(bar, NULL, count);
}

