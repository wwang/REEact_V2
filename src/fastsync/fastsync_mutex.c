/*
 * Implementation of the mutex for fast synchronization primitives.
 * This mutex is tree structured and optimized for NUMA systems.
 * Note that because this implementation uses separated queues for different 
 * nodes and cores, threads are not released in FIFO order. A global queue
 * should be used for that purposes (such as MCS lock).
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


#define FASTSYNC_MUTEX_SPIN_LOCK_LOOPS 100
#define FASTSYNC_MUTEX_CPU_YIELDS 5


/*
 * initialized a fastsync mutex object
 */
int fastsync_mutex_init(fastsync_mutex *m, const fastsync_mutex_attr *attr)
{
	if(m == NULL)
		return 1;

	m->state = 0;
	if(attr != NULL)
		m->parent = attr->parent;
	else
		m->parent = NULL;

	return 0;
}

/*
 * Lock a fastsync mutex; block if lock not acquired. This function first lock
 * the mutex at core-level. If succeed, it proceeds to lock at higher levels.
 */
int fastsync_mutex_lock(fastsync_mutex *mutex)
{
	int locked;
	int i;
	
	if(mutex == NULL)
		return 1;
	
	// try to lock the mutex
	locked = atomic_for(&(mutex->state), 1) & 1;

	// check if we have acquired the lock
	if(!locked){
		// lock acquired

		/*
		 * yield the processor several times to allow other threads 
		 * on this core who also want this mutex to show themselves;
		 * we want to have some threads waiting, so we don't have to
		 * release the mutex from this core too soon.
		 */
		for(i = 0; i < FASTSYNC_MUTEX_CPU_YIELDS; i++)
			sched_yield();
	}
	else{
		// lock not acquired
		
		/*
		 * set the lock stated to contended and suspend 
		 */
		while((atomic_xchg(&(mutex->state), 3) & 1))
			sys_futex(mutex, FUTEX_WAIT_PRIVATE, 3, NULL, NULL, 0);

		// lock acquired
	}

	// lock the parent mutex if necessary
	if(mutex->parent)
		fastsync_mutex_lock_interproc(mutex->parent);
	// all mutexes acquired
	return 0;

}

/*
 * lock mutex at inter-processor level
 */
int fastsync_mutex_lock_interproc(fastsync_mutex *mutex)
{
	int locked;
	int i;
	
	if(mutex == NULL)
		return 1;
	
	// spin and try to lock the mutex
	for (i = 0; i < FASTSYNC_MUTEX_SPIN_LOCK_LOOPS ; i++){
		locked = atomic_for(&(mutex->state), 1) & 1;
		if(!locked){
			// lock acquired
			// lock the parent mutex if necessary
			if(mutex->parent)
				fastsync_mutex_lock_interproc(mutex->parent);
			// all mutexes acquired
			return 0;
		}
		spinlock_hint();
	}
	
	//  block and wait for the lock
		
	/*
	 * set the lock stated to contended and suspend 
	 */
	while((atomic_xchg(&(mutex->state), 3) & 1))
		sys_futex(mutex, FUTEX_WAIT_PRIVATE, 3, NULL, NULL, 0);
	// lock acquired
	
	// lock the parent mutex if necessary
	if(mutex->parent)
		fastsync_mutex_lock_interproc(mutex->parent);
	// all mutexes acquired
	return 0;

}

/*
 * Unlock a fastsync mutex. This function first unlock the mutex at core level,
 * if there is any thread waiting on this core, the mutex is transferred to this
 * thread.
 */
int fastsync_mutex_unlock(fastsync_mutex *mutex)
{
	int waked;

	if(mutex == NULL)
		return 1;

	/* locked but not contended */
	if( mutex->state == 1 && (atomic_cmpxchg(&(mutex->state), 1, 0) == 1)){
		// mutex released
		/*
		 * note that a thread on this core can kick in at this point
		 * and tries to lock the mutex, and it will succeed; however,
		 * this thread may be blocked at a parent mutex; This is
		 * semantically correct, but incurs unnecessary context 
		 * switches.
		 */
		// release parent lock if necessary
		if(mutex->parent)
			return fastsync_mutex_unlock_interproc(mutex->parent);
	}
		
	/* locked and contended */
	mutex->state = 0; // the contended state will be set if next thread 
		          // wakes up for futex
	
	waked = sys_futex(mutex, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);

	if(waked == 1){
		// one thread wakes up, lock will be transferred
		return 0;
	}
	else if (waked == 0){
		// no thread wakes up ==> no thread waiting, release parent
		// lock
		if(mutex->parent)
			return fastsync_mutex_unlock_interproc(mutex->parent);
	}
	
	// waked if not 0 or 1, futex problem
	LOGERRX("Some thing wrong with futex wake (%d): ", waked);
	return 2;
}


/*
 * Unlock a fastsync mutex at inter-processor level; threads waiting at this 
 * level is released first (even if another tree at different tree-node waits
 * for this mutex before anyone on this particular tree-node.
 */
int fastsync_mutex_unlock_interproc(fastsync_mutex *mutex)
{
	int waked, i;

	if(mutex == NULL)
		return 1;

	/* locked but not contended */
	if( mutex->state == 1 && (atomic_cmpxchg(&(mutex->state), 1, 0) == 1)){
		// mutex released
		// release parent lock if necessary
		if(mutex->parent)
			return fastsync_mutex_unlock_interproc(mutex->parent);
	}
		
	/* locked and contended */
	atomic_fand(&(mutex->state), 0xFFFFFFFE); // unlock the lock
	
	/* spin and wait for some one to acquire it (wake-up throttling) */
	for (i = 0; i < FASTSYNC_MUTEX_SPIN_LOCK_LOOPS ; i++){
		if(mutex->state & 1){
			// lock transferred
			spinlock_hint();
			return 0;
		}
	}
		        
	// reset contended bit
	atomic_fand(&(mutex->state), 0xFFFFFFFD); // will be set by newly
                                                 // waken thread from futex

	waked = sys_futex(mutex, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);

	if(waked == 1){
		// one thread wakes up, lock will be transferred
		return 0;
	}
	else if (waked == 0){
		// no thread wakes up ==> no thread waiting, release parent
		// lock
		if(mutex->parent)
			return fastsync_mutex_unlock_interproc(mutex->parent);
		else 
			return 0;
	}
	
	// waked if not 0 or 1, futex problem
	LOGERRX("Some thing wrong with futex wake (%d): ", waked);
	return 2;
}
