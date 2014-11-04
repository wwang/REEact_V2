/*
 * Implementation of the barriers of the fast synchronization primitives.
 * Note that fastsync_barrier supports tree-barrier. However, Because the 
 * structure of the tree is application-specific, the construction of the tree
 * is left for the user. This implementation only deals with the generic 
 * algorithm of tree-barrier wait. 
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <limits.h>
#include <linux/futex.h>

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "fastsync.h"
#include "../utils/reeact_utils.h"

int fastsync_barrier_init(fastsync_barrier *barrier,
			  const fastsync_barrier_attr *attr, 
			  unsigned count)
{
	if(barrier == NULL)
		return 1;

	barrier->total_count = count;
	barrier->reset = 0;
	/* barrier->total_yield = 0; */
	barrier->parent_bar = NULL;

	DPRINTF("in fastsync barrier init with count %d.\n", 
		barrier->total_count);


	return 0;
}


/* 
 * This is the base level wait function, i.e., threads waiting at core-level
 * barrier call futex to block themselves.
 */
int fastsync_barrier_wait(fastsync_barrier *barrier)
{
	/*
	 * A typical implementation of "pool barrier"
	 */

	int ret_val = 1;


	int cur_seq = atomic_read(barrier->seq);
	// atomic add and fetch
	int count = atomic_add(&(barrier->waiting), 1);
	
	// done waiting for the barrier
	if(count == barrier->total_count){
		// wait for the parent barrier
		if(barrier->parent_bar)
			fastsynt_barrier_wait_interproc(barrier->parent_bar, 
							count);
		
		// this is the last thread hitting the barrier
		// clear the waiting count and increment sequence count
		// these operations should be done simultaneously 
		gcc_barrier();
		barrier->reset = barrier->seq + 1;
		gcc_barrier();
#ifdef _FUTEX_BARRIER_
		// wake up other threads waiting at this barrier
		if(barrier->total_count > 1)
			sys_futex(&barrier->seq, FUTEX_WAKE_PRIVATE,
				  INT_MAX, NULL, NULL, 0);
#endif
		ret_val = PTHREAD_BARRIER_SERIAL_THREAD;
		//break;
		return ret_val;
	}

	if(count < barrier->total_count){
		while (cur_seq == atomic_read(barrier->seq)) {
			/* barrier->total_yield++; */
#ifndef _FUTEX_BARRIER_
			sched_yield(); // give up processor
#else
			// block thread itself
			sys_futex(&barrier->seq, FUTEX_WAIT_PRIVATE,
				  cur_seq, NULL, NULL, 0);
#endif
		}
		// normal wait success
		ret_val = 0;
		//break;
		return ret_val;
	}
	
	/* should be unreachable */
	return ret_val;
}


/*
 * Barrier wait function for threads running on different processors/cores,
 * i.e., wait function at barriers higher than core-level barriers. Different
 * from the core-level barrier where blocked wait is used, inter-processor 
 * barriers are waiting with spinning, because spinning could be faster than
 * a futex system call.
 */
int fastsynt_barrier_wait_interproc(fastsync_barrier *barrier, int inc_count)
{
	int ret_val;

	int cur_seq = atomic_read(barrier->seq);
	// atomic add and fetch
	int count = atomic_add(&(barrier->waiting), inc_count);

		// done waiting for the barrier
	if(count == barrier->total_count){
		// wait at parent barrier
		if(barrier->parent_bar)
			fastsynt_barrier_wait_interproc(barrier->parent_bar,
							count);

		// this is the last thread hitting the barrier
		// clear the waiting count and increment sequence count
		// these operations should be done simultaneously 
		gcc_barrier();
		barrier->reset = barrier->seq + 1;
		gcc_barrier();
		ret_val = 0;
		//break;
		return ret_val;
	}

	if(count < barrier->total_count){
		while (cur_seq == atomic_read(barrier->seq)) {
			/* spin */
		}
		// normal wait success
		ret_val = 0;
		//break;
		return ret_val;
	}
	
	return ret_val;
}

int fastsync_barrier_destroy(fastsync_barrier *barrier)
{
	DPRINTF("in fastsync barrier destroy (%d syncs)\n", barrier->seq);

	return 0;
}

