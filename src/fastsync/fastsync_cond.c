/*
 * Implementation of the fast synchronization conditional variable
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

/*
 * Initialize a fastsync conditional variable object
 */
int fastsync_cond_init(fastsync_cond *cond, const fastsync_cond_attr *atrr)
{
	if(cond == NULL)
		return 1;

	cond->seq = 0;
	cond->mutex = NULL;
	
	return 0;
}


/*
 * Destroy a fastsync conditional variable object
 */
int fastsync_cond_destroy(fastsync_cond *cond)
{
	if(cond == NULL)
		return 1;

	cond->seq = 0;
	cond->mutex = NULL;
	
	return 0;
}

/*
 * wait on a conditional variable
 */
int fastsync_cond_wait(fastsync_cond *cond, fastsync_mutex *mutex)
{
	int cur_seq;
	fastsync_mutex *old;
	
	//if(cond == NULL || mutex == NULL)
	//	return 1;

	/* acquired current sequence number */
	cur_seq = cond->seq;
	
	if(cond->mutex != mutex){
		if(cond->mutex != NULL){
			/* mutex is different the previously used mutex */
			DPRINTF("cond->mutex is %p, input mutex is %p\n", cond->mutex,
				mutex);
			return 2;
		}

		/* set the mutex to the be the conditional variable's mutex */
		DPRINTF("reset the mutex with %p\n", mutex);
		old = atomic_cmpxchg(&(cond->mutex), NULL, mutex);
		if(old != NULL){
			DPRINTF("old mutex is NULL?: %p, input mutex is\n", old,
				mutex);
			return 2;
		}
	}
	
	/* release mutex */
	fastsync_mutex_unlock(mutex);
	/* wait on the conditional variable */
	sys_futex(&(cond->seq), FUTEX_WAIT_PRIVATE, cur_seq, NULL, NULL, 0);

	/*
	 * suspend if the mutex is lock
	 */
	while((atomic_xchg(&(mutex->state), 3) & 1)){
		sys_futex(&(mutex->state), FUTEX_WAIT_PRIVATE, 3, NULL, 
			  NULL, 0);
	}

	/* mutex acquired */
	return 0;
}

/* 
 * let at least one waiter proceed
 */
int fastsync_cond_signal(fastsync_cond *cond)
{
	if(cond == NULL)
		return 1;
	
	/* increase the sequence count */
	atomic_addf(&(cond->seq), 1);
	
	/* release on waiter */
	sys_futex(&(cond->seq), FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);

	return 0;
}

int fastsync_cond_signal_count(fastsync_cond *cond, int *count)
{

	//if(cond == NULL)
	//	return 1;
	
	/* increase the sequence count */
	atomic_addf(&(cond->seq), 1);
	
	/* release one waiter */
	*count = sys_futex(&(cond->seq), FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 
			   0);

	return 0;
}

/*
 * let all waiters proceed
 */
int fastsync_cond_broadcast(fastsync_cond *cond)
{
	int count = 0;

	if(cond == NULL)
		return 1;

	/* increase the sequence count */
	atomic_addf(&(cond->seq), 1);
	
	/* 
	 * release one waiter and put the rest on the mutex wait queue
	 */
	if(cond->mutex)
		count = sys_futex(&(cond->seq), FUTEX_REQUEUE_PRIVATE, 1, 
				  (void*)INT_MAX, cond->mutex, 0);

	return 0;
}
