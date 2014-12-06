/*
 * Implementation of the fast synchronization conditional variable. This 
 * conditional variable is distributed and tree structured. The first thread 
 * hits a tree-node can proceed to its parent, later come threads will wait at 
 * this tree-node, until the first thread is released from the parent. The first
 * thread then release every waiter on this tree-node.
 *
 * This type of distribution is good for high synchronization frequency and 
 * large scale machine. However, for low synchronization frequency, a non-
 * distributed is probably enough.
 *
 * Also, check out the comments of flex-pthread conditional variable 
 * implementation on how to structure distributed conditional variable.
 *
 * Moreover, futex is a heavy system call. With current fastsync_cond 
 * implementation, more threads than cores would suffer a lot from the futex 
 * overhead. To make things faster, scheduler support of FIFO or RR policy is 
 * required.
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
	
	if(cond == NULL || mutex == NULL)
		return 1;

	if(cond->mutex != mutex){
		if(cond->mutex != NULL){
			/* mutex is different the previously used mutex */
			LOGERR("cond->mutex is %p, input mutex is %p\n", 
			       cond->mutex, mutex);
			return 2;
		}

		/* set the mutex to the be the conditional variable's mutex */
		DPRINTF("reset the mutex with %p in cond %p\n", mutex, cond);
		old = atomic_cmpxchg(&(cond->mutex), NULL, mutex);
		if(old != NULL){
			LOGERR("old mutex is NULL?: %p, input mutex is\n", old,
				mutex);
			return 2;
		}
	}

	if(!cond->use_child && cond->parent){
		/* use parent conditional variable */
		/* 
		 * there is no need to do atomic checks and updates as a mutex
		 * should be acquired at this point.
		 */
		cond->use_child = 1;
		fastsync_cond_wait(cond->parent, mutex);
		/* release conditional variable on this node */
		/* 
		 * again, no need for atomic operations as the mutex is acquired
		 */
		cond->seq++;
		/*
		 * I have to release at least one thread. It seems the kernel
		 * may delay thread re-queue to even after the mutex is 
		 * released.
		 */
		sys_futex(&(cond->seq), FUTEX_REQUEUE_PRIVATE, 1,
			  (void*)INT_MAX, mutex, 0);
		cond->use_child = 0;
		return 0;
	}

	/* acquired current sequence number */
	cur_seq = cond->seq;
	
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
	if(cond == NULL)
		return 1;

	/* increase the sequence count */
	atomic_addf(&(cond->seq), 1);
	
	/* 
	 * release one waiter and put the rest on the mutex wait queue
	 */
	if(cond->mutex)
		sys_futex(&(cond->seq), FUTEX_REQUEUE_PRIVATE, 1, 
			  (void*)INT_MAX, cond->mutex, 0);

	return 0;
}
