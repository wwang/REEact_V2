/*
 * Implementation of the mutex for fast synchronization primitives.
 *
 * Unfortunately it turns out that the previous version of tree-mutex is too
 * slow for real use. The problem is that the sequence of the tree nodes being 
 * released. For a hierarchical mutex to work, the mutexes have to be unlocked
 * in the reverse order of which they are locked. However, to reduce the 
 * global broadcast and contention for shared memory (of the mutex), one would
 * prefer to release the mutexes in the same order as they are locked, so that
 * a mutex could be transferred to a close thread (on the same core or node or
 * socket). Unfortunately, this kind of implementation is too complicate to be
 * fast. Additionally, there is a risk of starving other cores and nodes. 
 * 
 * Besides being complicated, on NUMA machines a released mutex is naturally
 * transferred to a close core/node because the close cores/nodes receives the 
 * shared memory faster than remoter cores/nodes. So they would have higher
 * chance of grabbing it. 
 *
 * Because of the above two reasons I chose the simplest implementation here.
 * However, it worth to note that a tree-structured mutex can still performs 
 * very well, because of the reduced number of shared memory broadcast. This 
 * behavior can be easily observed by using pthread_mutex and running more 
 * threads than cores. About 30% speedup can be achieved. Please refer to 
 * a PACT 2011 Pusukuri et al., and a later pager from the same authors
 * at HotOS for more about this behavior. My current experience is that 
 * implementing such a tree-mutex with simple algorithm requires scheduler's 
 * help: I need a FIFO or RR scheduling policy to get it work.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <limits.h>
#include <linux/futex.h>
#include <errno.h>

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include "fastsync.h"
#include "../utils/reeact_utils.h"

/*
 * the more threads per core, the more cores, the smaller this number should be.
 */
#define FASTSYNC_MUTEX_SPIN_LOCK_LOOPS 1

/*
 * initialized a fastsync mutex object
 */
int fastsync_mutex_init(fastsync_mutex *m, const fastsync_mutex_attr *attr)
{
	if(m == NULL)
		return 1;

	m->state = 0;

	return 0;
}

/*
 * Lock a fastsync mutex
 */
int fastsync_mutex_lock(fastsync_mutex *mutex)
{
	int locked;
	int i;

	//sched_yield();
	
	if(mutex == NULL)
		return 1;
	
	/* spin and try to lock the mutex */
	for (i = 0; i < FASTSYNC_MUTEX_SPIN_LOCK_LOOPS ; i++){
		locked = atomic_for(&(mutex->state), 1) & 1;
		if(!locked){
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

	return 0;
}

/*
 * Unlock a fastsync mutex.
 */
int fastsync_mutex_unlock(fastsync_mutex *mutex)
{
	int i;

	if(mutex == NULL)
		return 1;

	
	/* locked but not contended */
	if( mutex->state == 1 && (atomic_cmpxchg(&(mutex->state), 1, 0) == 1)){
		return 0;
	}
		
	/* locked and contended */
	atomic_fand(&(mutex->state), 0xFFFFFFFE); // unlock the lock
	
	/* spin and wait for some one to acquire it (wake-up throttling) */
	for (i = 0; i < FASTSYNC_MUTEX_SPIN_LOCK_LOOPS ; i++){
		if(mutex->state & 1){
			/* lock transferred */
			return 0;
		}
		spinlock_hint();
	}
		        
	/* reset contended bit */
	atomic_fand(&(mutex->state), 0xFFFFFFFD); // will be set by newly
                                                 // waken thread from futex
	
	sys_futex(&(mutex->state), FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0);


	return 0;
}


/*
 * Lock a fastsync mutex
 */
int fastsync_mutex_trylock(fastsync_mutex *mutex)
{
	int locked;

	//sched_yield();
	
	if(mutex == NULL)
		return 1;
	
	/* try to lock the mutex */
	locked = atomic_for(&(mutex->state), 1) & 1;
	if(!locked){
		return 0;
	}
	
	return EBUSY;
}
