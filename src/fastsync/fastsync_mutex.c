/*
 * Implementation of the mutex for fast synchronization primitives.
 * This mutex is tree structured and optimized for NUMA systems.
 * Note that because this implementation uses separated queues for different 
 * nodes and cores, threads are not released in FIFO order. A global queue
 * should be used for that purposes (such as MCS lock).
 *
 * The tricky part of the implementation: For standard hierarchical mutexes, the
 * mutexes have to be unlocked in the reverse order as these mutexes are locked
 * to avoid dead locking. However, since I want to transfer locks to local
 * threads first, I have to unlock these mutexes in the same order as they are
 * locked. Therefore, I need to keep an id of the owner of each mutex. 
 *
 * The implementation should be a lot easier if the scheduler is involved.
 *
 * Currently I have only two-levels of mutexes tested and implemented. It is
 * possible to have multiple levels. However, the caller of the lock function
 * has to supplied a list of thread, core, node, socket ids to be used with
 * the mutex at corresponding levels.
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


#define FASTSYNC_MUTEX_SPIN_LOCK_LOOPS 20
#define FASTSYNC_MUTEX_CPU_YIELDS 10
#define FASTSYNC_MUTEX_NO_OWNER 0xffffffff

/*
 * initialized a fastsync mutex object
 */
int fastsync_mutex_init(fastsync_mutex *m, const fastsync_mutex_attr *attr)
{
	if(m == NULL)
		return 1;

	m->state = 0;
	m->cur_owner = FASTSYNC_MUTEX_NO_OWNER;
	m->next_thr_owner = FASTSYNC_MUTEX_NO_OWNER;
	m->next_owner_transfer_lock = 0;
	m->wakeup_seq = 0;
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
int fastsync_mutex_lock(fastsync_mutex *mutex, int thr_id, int core_id)
{
	int locked;
	int i;
	
	if(mutex == NULL)
		return 1;

	if(mutex->cur_owner != thr_id){
		
		/* try to lock the real mutex */
		locked = atomic_for(&(mutex->state), 1) & 1;
		
		// check if we have acquired the lock
		if(!locked){
			/* lock acquired */
			/* set the owner ship of the lock */
			mutex->cur_owner = thr_id;

			/*
			 * yield the processor several times to allow other
			 * threads on this core who also want this mutex to show
			 * themselves; we want to have some threads waiting, so 
			 * we don't have to release the mutex from this core too
			 * soon.
			 */
			for(i = 0; i < FASTSYNC_MUTEX_CPU_YIELDS; i++)
				sched_yield();
		}
		else{
			/*  failed to acquired lock */
			/*
			 * set the lock stated to contended and suspend 
			 */
			while((atomic_xchg(&(mutex->state), 3) & 1)){
				sys_futex(&(mutex->state), FUTEX_WAIT_PRIVATE, 
					  3, NULL, NULL, 0);
			}
			
			/* set the owner */
			mutex->cur_owner = thr_id;
			//return 0;
		}
	}

	// lock the parent mutex if necessary
	if(mutex->parent){
		fastsync_mutex_lock_interproc(mutex->parent, mutex, thr_id, 
					      core_id);
	}

	// all mutexes acquired
	return 0;

}

/*
 * lock mutex at inter-processor level
 */
int fastsync_mutex_lock_interproc(fastsync_mutex *mutex, fastsync_mutex *child,
				  int thr_id, int core_id)
{
	int locked;
	int i;
	int owner_lock;
	int lock_transferred = 0;
	
	if(mutex == NULL)
		return 1;
	
	/*
	 * try to grab the lock if it have locked by a thread from this core
	 * before
	 */
	if(mutex->cur_owner == core_id){
		/* try to grab the ownership */
		owner_lock = atomic_xchg(&(child->next_owner_transfer_lock),1);
		if(owner_lock == 0){
			/* ownership lock grabbed; claim ownership */
			if(atomic_read(mutex->cur_owner) == core_id){
				child->next_thr_owner = thr_id;
				lock_transferred = 1;
			}
			/* unlock the mutex */
			child->next_owner_transfer_lock = 0;
		}
		lock_transferred = 1;
	}

	/*
	 * ownership of the lock did not transfer, grab the mutex normally.
	 */
	if(!lock_transferred){
		/* spin and try to lock the mutex */
		for (i = 0; i < FASTSYNC_MUTEX_SPIN_LOCK_LOOPS ; i++){
			locked = atomic_for(&(mutex->state), 1) & 1;
			if(!locked){
				/* set the owner */
				mutex->cur_owner = core_id;
				child->next_thr_owner = thr_id;
				/* lock the parent mutex if necessary */
				if(mutex->parent)
					fastsync_mutex_lock_interproc
						(mutex->parent, mutex, thr_id, 
						 core_id);
				/* all mutexes acquired */
				return 0;
			}
			spinlock_hint();
		}
	
		/* block and wait for the lock */
		
		/*
		 * set the lock stated to contended and suspend 
		 */
		while((atomic_xchg(&(mutex->state), 3) & 1)){
			sys_futex(&(mutex->state), FUTEX_WAIT_PRIVATE, 3, NULL, 
				  NULL, 0);
		}
		/* set the owner */
		mutex->cur_owner = core_id;
		child->next_thr_owner = thr_id;
	}
	

	/* lock the parent mutex if necessary */
	if(mutex->parent)
		fastsync_mutex_lock_interproc(mutex->parent, mutex,
					      thr_id, core_id);
	/* all mutexes acquired */
	return 0;

}

/*
 * Unlock a fastsync mutex. This function first unlock the mutex at core level,
 * if there is any thread waiting on this core, the mutex is transferred to this
 * thread.
 */
int fastsync_mutex_unlock(fastsync_mutex *mutex, int thr_id, int core_id)
{
	int waked;
	int wake_seq;

	if(mutex == NULL)
		return 1;

	/* unset the owner */
	mutex->cur_owner = FASTSYNC_MUTEX_NO_OWNER;

	/* locked but not contended */
	if( mutex->state == 1 && (atomic_cmpxchg(&(mutex->state), 1, 0) == 1)){
		/* mutex released */
		/*
		 * note that a thread on this core can kick in at this point
		 * and tries to lock the mutex, and it will succeed; however,
		 * this thread may be blocked at a parent mutex; This is
		 * semantically correct, but incurs unnecessary context 
		 * switches.
		 */
		/* release parent lock if necessary */
		/* 
		 * WARNING: Potential race condition here!!! Between the 
		 * unlocking of the core-level mutex and the unlocking of the
		 * global mutex, there is a chance that some thread on the same
		 * core can kick in and run, and take over the global lock. Then
		 * if the code keeps going here, the global lock will be 
		 * released before the second thread is done with it. 
		 *
		 * With the use of owner_transfer_lock, there should be NO race
		 * condition any more.
		 */
		if(mutex->parent)
			fastsync_mutex_unlock_interproc(mutex->parent, mutex,
							thr_id, core_id);
		return 0;
	}
	
	/* locked and contended */
	wake_seq = mutex->wakeup_seq;
	mutex->state = 0; // the contended state will be set if next thread 
		          // wakes up for futex

	waked = sys_futex(&(mutex->state), FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 
			  0);

	if(waked == 1){
		/*
		 * one thread wakes up, lock will be transferred
		 * give up the processor so the other threads can take the lock
		 * over. If this thread keeps running before other threads that
		 * are waiting for the mutex, the mutex are held by the core of 
		 * these threads, and threads on the other cores would starve.
		 * Therefore current thread is suspended on a wait queue. And
		 * is released when this mutex is released from this core.
		 */
		sys_futex(&(mutex->wakeup_seq), FUTEX_WAIT_PRIVATE, wake_seq, 
			  NULL, NULL, 0);
		//sched_yield();
		return 0;
	}
	else if (waked == 0){
		/*
		 * no thread wakes up ==> no thread waiting, release parent
		 * lock and wake up all threads on this core who have released
		 * the lock previously
		 */
		mutex->wakeup_seq++;
		sys_futex(&(mutex->wakeup_seq), FUTEX_WAKE_PRIVATE, INT_MAX, 
			  NULL, NULL, 0);
		if(mutex->parent)
			fastsync_mutex_unlock_interproc(mutex->parent, mutex, 
							thr_id, core_id);
		
		return 0;
	}
	
	/* waked if not 0 or 1, futex problem */
	LOGERRX("Some thing wrong with futex wake (%d): ", waked);
	return 2;
}


/*
 * Unlock a fastsync mutex at inter-processor level; threads waiting at this 
 * level is released first (even if another tree at different tree-node waits
 * for this mutex before anyone on this particular tree-node.
 */
int fastsync_mutex_unlock_interproc(fastsync_mutex *mutex, 
				    fastsync_mutex *child, int thr_id, 
				    int core_id)
{
	int waked;
	int i;
	int owner_lock;

	if(mutex == NULL)
		return 1;
	
	/* only release this thread if it is owned by this thread */
	owner_lock = atomic_xchg(&(child->next_owner_transfer_lock), 1);
	if(owner_lock == 1){
		/* 
		 * ownership being transferred to other thread, we cannot unlock
		 * it 
		 */
		//DPRINTF("Cannot  to release mutex %p for core %d by thread %d\n",
		//mutex, core_id, thr_id);
		return 0;
	}
	/* ownership lock grabbed */
	if(child->next_thr_owner != thr_id){
		/* 
		 * ownership transferred to other thread, we cannot unlock it; 
		 * just release the ownership lock and return
		 */
		child->next_owner_transfer_lock = 0;
		return 0;
		
	}
	else{
		/* this thread own the mutex, safe to unlock it */
		mutex->cur_owner = FASTSYNC_MUTEX_NO_OWNER;
		child->next_thr_owner = FASTSYNC_MUTEX_NO_OWNER;
		/* release the ownership lock */
		child->next_owner_transfer_lock = 0;
	}

	/* locked but not contended */
	if( mutex->state == 1 && (atomic_cmpxchg(&(mutex->state), 1, 0) == 1)){
		// mutex released
		// release parent lock if necessary
		if(mutex->parent)
			fastsync_mutex_unlock_interproc(mutex->parent, mutex, 
							thr_id, core_id);
		return 0;
	}
		
	/* locked and contended */
	atomic_fand(&(mutex->state), 0xFFFFFFFE); // unlock the lock
	
	/* spin and wait for some one to acquire it (wake-up throttling) */
	for (i = 0; i < FASTSYNC_MUTEX_SPIN_LOCK_LOOPS ; i++){
		if(mutex->state & 1){
			/* lock transferred */
			spinlock_hint();
			return 0;
		}
	}
		        
	/* reset contended bit */
	atomic_fand(&(mutex->state), 0xFFFFFFFD); // will be set by newly
                                                 // waken thread from futex
	
	waked = sys_futex(&(mutex->state), FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 
			  0);

	if(waked == 1){
		/* one thread wakes up, lock will be transferred */
		return 0;
	}
	else if (waked == 0){
		/* 
		 * no thread wakes up ==> no thread waiting, release parent lock
		 */
		if(mutex->parent)
			fastsync_mutex_unlock_interproc(mutex->parent, mutex, 
							thr_id, core_id);
		return 0;
	}
	
	// waked if not 0 or 1, futex problem
	LOGERRX("Some thing wrong with futex wake (%d): ", waked);
	return 2;
}
