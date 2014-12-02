/*
 * Mutex implementation for flex-pthread.
 *
 * Similar to barriers, mutex is also implemented as a tree. Threads have to 
 * completely locally for the mutexes representing cores and nodes first to
 * finally compete for the global mutex. Note that this implementation is 
 * like a spin-lock in that the threads do not acquire the mutex in a FIFO 
 * order; once a mutex is released, threads on the same core have the priority
 * of gaining it first, then the threads on the same node, and etc.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <common_toolx.h>
#include <simple_hashx.h>

#include "../../reeact.h"
#include "../../utils/reeact_utils.h"
#include "../../fastsync/fastsync.h"
#include "../reeact_policy.h"
#include "../../pthread_hooks/pthread_hooks_originals.h"

#include "flex_pthread.h"
#include "flexpth_mutex.h"
#include "flexpth_thread_keeper.h"

/* magic number indicates a pthread_mutex object is actually a flexpth mutex */
#define FLEXPTH_MUTEX_MAGIC_NUMBER1 31626262 
/* magic number indicates a flexpth mutex being initialized */
#define FLEXPTH_MUTEX_MAGIC_NUMBER2 50567133

/*
 * external data structure that holds the tree structure representing the 
 * processor topology
 */
extern struct _flexpth_bar_slist _bar_slist;

/*
 * two-way mapping that maps a core id to its corresponding element in the
 * static list and vice-versa.
 */
extern int *_core_to_list_map; // core to list element mapping
extern int *_list_to_core_map; // list element to core mapping
extern int total_core_cnt; // the total number of cores

/*
 * flex-pthread mutex initialization
 */
int flexpth_mutex_internal_init(void *data)
{
	return 0;
}

/*
 * flex-pthread mutex cleanup
 */
int flexpth_mutex_internal_cleanup(void *data)
{
	return 0;
}


/*
 * Initialize a new flex_pthread tree mutex
 */
int flexpth_tree_mutex_init(void *data,
			    struct flexpth_tree_mutex **tmutex,
			    struct flexpth_tree_mutex_attr *attr)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh;
	fastsync_mutex *mutexes;
	fastsync_mutex_attr mutex_attr;
	int i;
	struct flexpth_tree_mutex *tm;

	/*
	 * parameter checking
	 */
	if(rh == NULL || tmutex == NULL){
		LOGERR("reeact data (%p) and/or tmutex (%p) is NULL\n", rh, 
		       tmutex);
		return 1;
	}
	fh = (struct flexpth_data*)rh->policy_data;
	if(fh == NULL){
		LOGERR("policy data (%p) is NULL\n", fh);
		return 1;
	}

	/*
	 * the topology information should be ready for now
	 */
	if(_bar_slist.elements == NULL){
		LOGERR("topology information is not ready");
		return 2;
	}

	/*
	 * allocate space for the tree mutex
	 */
	tm = (struct flexpth_tree_mutex*)
		calloc(1, sizeof(struct flexpth_tree_mutex));
	*tmutex = tm;
	if(tm == NULL){
		LOGERRX("Unable to allocate space for tree mutex\n");
		return 3;
	}
	
	/*
	 * allocate spaces for the fastsync_mutex
	 */
	mutexes = (fastsync_mutex*)calloc(_bar_slist.len, 
					     sizeof(fastsync_mutex));
	if(mutexes == NULL){
		LOGERRX("Unable to allocate space for mutexes\n");
		return 3;
	}
	
	// initialize each mutex with proper parent information
	for(i = 1; i < _bar_slist.len; i++){
		mutex_attr.parent = mutexes; // there is one parent for all cores
		fastsync_mutex_init(mutexes + i, &mutex_attr);
		tm->mutexes[i] = (void*)(mutexes + i);
	}
	// set parent of the root barrier to NULL and total_count to count
	tm->mutexes[0] = mutexes;
	mutexes[0].parent = NULL;

#ifdef _REEACT_DEBUG_
	/*
	 * log the tree barrier array
	 */
	DPRINTF("Tree mutex created, array is:\n");
	for(i = 0; i < _bar_slist.len; i++){
		fprintf(stderr, "\t element %d (%p) parent %p state %d\n",
			i, mutexes + i, mutexes[i].parent, mutexes[i].state);
	}
	fprintf(stderr, "\n");
#endif
	
	return 0;

}

/*
 * wrapper functions for pthread_mutex functions
 */ 
extern struct reeact_data *reeact_handle; // the global REEact data handle
extern __thread struct flexpth_thread_info* self; // information of this thread
extern __thread long long barrier_idx; 

int flexpth_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr)
{
	struct flexpth_mutex *mutex = (struct flexpth_mutex*)m;
	int ret_val;

	if(mutex == NULL)
		return EINVAL;

	mutex->magic_number = FLEXPTH_MUTEX_MAGIC_NUMBER1;
	
	ret_val = flexpth_tree_mutex_init((void*)reeact_handle, 
					  &(mutex->tmutex), NULL);

	if(ret_val == 2)
		return EAGAIN;

	if(ret_val == 3)
		return ENOMEM;
	
	return 0;
}

/*
 * mutual exclusive initialized for flex-pthread mutex. This function is called
 * in mutex_lock or unlock when the mutex is not actually initialized. Only one
 * thread will be able to initialize it.
 * 
 * Note that there is a gap between the test of cur_magic and compare-and-swap.
 * if some thread other than flex-pthread managed threads changed the magic 
 * number during this gap, then we are in trouble.
 *
 * Input parameters:
 *     mutex: the to initialize
 */
int flexpth_mutex_init_critical(struct flexpth_mutex *mutex)
{
	int cur_magic = mutex->magic_number;

	/* try to lock this mutex for initialization */
	if(cur_magic != FLEXPTH_MUTEX_MAGIC_NUMBER1 &&
	   cur_magic != FLEXPTH_MUTEX_MAGIC_NUMBER2 &&
	   atomic_bool_cmpxchg(&(mutex->magic_number), cur_magic, 
			       FLEXPTH_MUTEX_MAGIC_NUMBER2)){
		// initialized the mutex
		flexpth_tree_mutex_init((void*)reeact_handle, 
					&(mutex->tmutex), NULL);
		mutex->magic_number = FLEXPTH_MUTEX_MAGIC_NUMBER1;
		// will be problematic if tree_mutex_init has error

		return 0;
	}
	else{
		// the mutex is being initialized, wait for 
		while(atomic_read(mutex->magic_number) != 
		      FLEXPTH_MUTEX_MAGIC_NUMBER1)
			spinlock_hint();
		// mutex initialized
		return 0;
	}
	/* not reached */
}

int flexpth_mutex_lock(pthread_mutex_t *m)
{
	struct flexpth_mutex *mutex = (struct flexpth_mutex*)m;
	int core_id;
	fastsync_mutex *fastm;
	int ret_val;

	if(mutex == NULL)
		return EINVAL;

	if(mutex->magic_number != FLEXPTH_MUTEX_MAGIC_NUMBER1){
		ret_val = flexpth_mutex_init_critical(mutex);
		if(ret_val){
			// unable to initialized the mutex
			LOGERR("Unable to initialized flexpth mutex\n");
			return EINVAL;
		}
	}
	

	/* locate the mutex corresponds to the running core of this thread */
	core_id = barrier_idx & 0x00000000ffffffff;
	fastm = (fastsync_mutex*)
		mutex->tmutex->mutexes[_core_to_list_map[core_id]];
	
	
	/* lock the mutex */
	return fastsync_mutex_lock(fastm, self->tidx, core_id);
}


int flexpth_mutex_unlock(pthread_mutex_t *m)
{
	struct flexpth_mutex *mutex = (struct flexpth_mutex*)m;
	int core_id;
	fastsync_mutex *fastm;

	if(mutex == NULL)
		return EINVAL;

	if(mutex->magic_number != FLEXPTH_MUTEX_MAGIC_NUMBER1)
		// unlocking a non-fastsync or uninitialized mutex
		return 0;

	/* locate the mutex corresponds to the running core of this thread */
	core_id = barrier_idx & 0x00000000ffffffff;
	fastm = (fastsync_mutex*)
		mutex->tmutex->mutexes[_core_to_list_map[core_id]];
       
	
	/* lock the mutex */
	return fastsync_mutex_unlock(fastm, self->tidx, core_id);
}

int flexpth_mutex_destroy(pthread_mutex_t *m)
{
	struct flexpth_mutex *mutex = (struct flexpth_mutex*)m;

	if(mutex == NULL)
		return EINVAL;

	if(mutex->magic_number != FLEXPTH_MUTEX_MAGIC_NUMBER1)
		// destroying a non-fastsync mutex
		return 0;

	// TODO: implement the mutex destroy
	LOGERR("FLEXPTH mutex destroy not implemented\n");
	
	return 0;
}

int flexpth_mutex_trylock(pthread_mutex_t *m)
{
	// TODO: implement the mutex try lock
	LOGERR("FLEXPTH mutex try lock not implemented\n");

	return EINVAL;
}

int flexpth_mutex_timedlock(pthread_mutex_t *m, 
			   const struct timespec *abs_timeout)
{
	// TODO: implement the mutex timed lock
	LOGERR("FLEXPTH mutex timed lock not implemented\n");

	return EINVAL;
}

int flexpth_mutex_consistent(pthread_mutex_t *m)
{
	// TODO: implement the mutex consistent
	LOGERR("FLEXPTH mutex consistent not implemented\n");

	return EINVAL;
}
