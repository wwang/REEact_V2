/*
 * Conditional variable implementation for flex-pthread. Flex-pthread 
 * conditional variable is distributed and can be configured into different 
 * structures: 
 *   1. no distribution just one plain conditional variable
 *   2. fully distributed. one conditional variable per core, then one per
 *      node, and one per socket, and one per two sockets, and one global.
 *      all organized in a tree-structure to reflect processor topology.
 *   3. fully distributed with two levels of conditional variables: one per
 *      each core, then one global.
 *   4. statically distributed. User may specified the number of conditional
 *      variables. Each core will be assigned on of the conditional variable.
 *      In signal or broadcast, every conditional variables are signaled or
 *      broadcasted. There is no tree structure.
 * These structures can be configured by the setting the macros 
 * FLEXPTH_COND_DISTRIBUTE_LEVEL and FLEXPTH_COND_TWO_LEVEL_DISTRIBUTION.
 * Which structure performs faster depends on the number of threads and 
 * synchronization frequency and processor topology. Only experiment can tell.
 * For low synchronization frequency, "no distribution" is probably the best for
 * its simplicity. For high synchronization frequency, full distributed is 
 * probably the best. Still, only experiments can tell.
 * 
 * It is also worth nothing that if there are more threads than cores, many 
 * threads would not even call cond_wait as the condition is probably met before
 * the call (i.e., the while loop condition before each cond_wait is false). 
 *
 * Moreover, futex is a heavy system call. With current fastsync_cond 
 * implementation, more threads than cores would suffer a lot from the futex 
 * overhead. To make things really simple, scheduler support of FIFO or RR 
 * policy is required.
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
#include "flexpth_cond.h"
#include "flexpth_thread_keeper.h"

/* magic number indicates a pthread_cond object is actually a flexpth mutex */
#define FLEXPTH_COND_MAGIC_NUMBER1 31626263
/* magic number indicates a flexpth cond being initialized */
#define FLEXPTH_COND_MAGIC_NUMBER2 50567134
/* 
 * how many distributed variable to use:
 * 0: fully distributed (per-core conditional variable)
 * 1: no distribution
 * other: static distribution level
 */
#define FLEXPTH_COND_DISTRIBUTE_LEVEL 4
/*
 * whether use only two level of distributions, global and per-core; only useful
 * for full distributed conditional variable
 */  
#define FLEXPTH_COND_TWO_LEVEL_DISTRIBUTION 0

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
int flexpth_cond_internal_init(void *data)
{
	return 0;
}

/*
 * flex-pthread mutex cleanup
 */
int flexpth_cond_internal_cleanup(void *data)
{
	return 0;
}


/*
 * Initialize a new flex_pthread distributed conditional variable
 */
int flexpth_distribute_cond_init(void *data, struct flexpth_cond *cond,
			    struct flexpth_cond_attr *attr)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	fastsync_cond *conds = NULL;
	int i;

	/*
	 * parameter checking
	 */
	if(rh == NULL || cond == NULL){
		LOGERR("reeact data (%p) and/or cond (%p) is NULL\n", rh,
		       cond);
		return 1;
	}

	if(_bar_slist.len == 0){
		LOGERR("processor topology information not ready\n");
		return 1;
	}

	/*
	 * allocate space for the fastsync_cond
	 */ 
	conds = (fastsync_cond*) calloc(_bar_slist.len, sizeof(fastsync_cond));
	if(conds == NULL){
		LOGERRX("Unable to allocate space for flex-pthread mutex: ");
		return 2;
	}
	cond->conds = (void*)conds;
	cond->len = _bar_slist.len;
	// set the parent pointers of the conditional variables
	for(i = 1; i < _bar_slist.len; i++){
#if FLEXPTH_COND_DISTRIBUTE_LEVEL > 1
		/* static distribution*/
		DPRINTF("static distribution\n");
		conds[i].parent = NULL;
#elif FLEXPTH_COND_TWO_LEVEL_DISTRIBUTION == 1
		/* use two level distribution */
		DPRINTF("two level distribution\n");
		conds[i].parent = conds;
#else
		DPRINTF("multiple-level distribution\n");
		/* use multiple level distribution */
		conds[i].parent = conds +  _bar_slist.elements[i];
#endif
	}
	// set parent of the root conditional variable to NULL a
	conds[0].parent = NULL;

#ifdef _REEACT_DEBUG_
	/*
	 * log the tree barrier array
	 */
	DPRINTF("Distributed conditional variable created, array is:\n");
	for(i = 0; i < _bar_slist.len; i++){
		fprintf(stderr, "\t element %d (%p) parent %p\n",
			i, conds + i, conds[i].parent);
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

int flexpth_cond_init(pthread_cond_t *cv, const pthread_condattr_t *attr)
{
	struct flexpth_cond *cond = (struct flexpth_cond*)cv;
	int ret_val;

	if(cond == NULL)
		return EINVAL;

	cond->magic_number = FLEXPTH_COND_MAGIC_NUMBER1;
	
	ret_val = flexpth_distribute_cond_init(reeact_handle, cond, NULL);

	if(ret_val == 2)
		return EAGAIN;

	if(ret_val == 3)
		return ENOMEM;
	
	return 0;
}

/*
 * mutual exclusive initialized for flex-pthread cond. This function is called
 * the first time the conditional variable is accessed if it is not actually 
 * initialized. Only one thread will be able to initialize it.
 * 
 * Note that there is a gap between the test of cur_magic and compare-and-swap.
 * if some thread other than flex-pthread managed threads changed the magic 
 * number during this gap, then we are in trouble. Although this should never
 * happen in correct codes.
 *
 * Input parameters:
 *     cond: the to initialize
 */
int flexpth_cond_init_critical(struct flexpth_cond *cond)
{
	int cur_magic = cond->magic_number;

	/* try to lock this mutex for initialization */
	if(cur_magic != FLEXPTH_COND_MAGIC_NUMBER1 &&
	   cur_magic != FLEXPTH_COND_MAGIC_NUMBER2 &&
	   atomic_bool_cmpxchg(&(cond->magic_number), cur_magic, 
			       FLEXPTH_COND_MAGIC_NUMBER2)){
		// initialized the mutex
		flexpth_distribute_cond_init(reeact_handle, cond, NULL);
		cond->magic_number = FLEXPTH_COND_MAGIC_NUMBER1;
		// will be problematic if cond_init has error
		DPRINTF("flexpth conditional variable critically "
			"initialized\n");
		return 0;
	}
	else{
		// the mutex is being initialized, wait for 
		while(atomic_read(cond->magic_number) != 
		      FLEXPTH_COND_MAGIC_NUMBER1)
			spinlock_hint();
		// mutex initialized
		return 0;
	}
	/* not reached */
}

int flexpth_cond_wait(pthread_cond_t *cv, pthread_mutex_t *m)
{
	struct flexpth_mutex *mutex = (struct flexpth_mutex*)m;
	struct flexpth_cond *cond = (struct flexpth_cond*)cv;
	fastsync_cond *fcv = NULL;
	int core_id = -1;
	int ret_val;
	

	if(mutex == NULL || cond == NULL)
		return EINVAL;

	if(cond->magic_number != FLEXPTH_COND_MAGIC_NUMBER1){
		ret_val = flexpth_cond_init_critical(cond);
		if(ret_val){
			// unable to initialized the mutex
			LOGERR("Unable to initialized flexpth cond\n");
			return EINVAL;
		}
		/* 
		 * there is no need to check the mutex. because the
		 * mutex has to lock when reaching here, it should have
		 * been properly initialized
		 */
	}
	
	/* find the conditional variable for this core */
#if FLEXPTH_COND_DISTRIBUTE_LEVEL == 0
	/* fully distribution */
	DPRINTF("full distribute\n");
	core_id = barrier_idx & 0x00000000ffffffff; 
	fcv  = (fastsync_cond*)cond->conds + _core_to_list_map[core_id];	
	ret_val = fastsync_cond_wait(fcv, (fastsync_mutex*)mutex->mutex);
#elif FLEXPTH_COND_DISTRIBUTE_LEVEL == 1
	/* no distribution */
	DPRINTF("no distribute\n");
	ret_val = fastsync_cond_wait((fastsync_cond*)cond->conds, 
				     (fastsync_mutex*)mutex->mutex);
#else 
	/* static distribution */
	DPRINTF("static distribute\n");
	core_id = (barrier_idx & 0x00000000ffffffff) %
         	FLEXPTH_COND_DISTRIBUTE_LEVEL;
	fcv  = (fastsync_cond*)cond->conds + core_id;
	ret_val = fastsync_cond_wait(fcv, (fastsync_mutex*)mutex->mutex);
#endif
	
	if(ret_val != 0)
		LOGERR("thread %d leave the cond var %p (%p) with ret %d, "
		       "core id is %d\n", self->tidx, fcv, cond->conds, ret_val,
			_core_to_list_map[core_id]);

	return 0;

}


int flexpth_cond_signal(pthread_cond_t *cv)
{
	struct flexpth_cond *cond = (struct flexpth_cond*)cv;
	int ret_val;

	if(cond == NULL)
		return EINVAL;

	if(cond->magic_number != FLEXPTH_COND_MAGIC_NUMBER1){
		ret_val = flexpth_cond_init_critical(cond);
		if(ret_val){
			// unable to initialized the mutex
			LOGERR("Unable to initialized flexpth cond\n");
			return EINVAL;
		}
	}

	/* let the waiters go core by core */
#if FLEXPTH_COND_DISTRIBUTE_LEVEL == 0
	/* fully distribute */
	fastsync_cond_signal((fastsync_cond*)cond->conds);
#elif FLEXPTH_COND_DISTRIBUTE_LEVEL == 1
	/* no distribute */
	fastsync_cond_signal((fastsync_cond*)cond->conds);
#else 
	/* static distribute */
	{
		int i;
		for(i = 0; i < FLEXPTH_COND_DISTRIBUTE_LEVEL; i++)
			fastsync_cond_signal((fastsync_cond*)(cond->conds) + i);
	}
#endif
	
	return 0;
}

int flexpth_cond_destroy(pthread_cond_t *cv)
{
	struct flexpth_cond *cond = (struct flexpth_cond*)cv;

	if(cond == NULL)
		return EINVAL;

	if(cond->magic_number != FLEXPTH_COND_MAGIC_NUMBER1)
		/* destroying a non-fastsync conditional variable */
		return 0;
 
	/* free the memory and reset counters */
	cond->len = -1;
	cond->first_core_idx = -1;
	if(cond->conds)
		free(cond->conds);
	cond->conds = NULL;
	cond->magic_number = 0;
	
	return 0;
}

int flexpth_cond_broadcast(pthread_cond_t *cv)
{
	struct flexpth_cond *cond = (struct flexpth_cond*)cv;
	int ret_val;

	if(cond == NULL)
		return EINVAL;

	if(cond->magic_number != FLEXPTH_COND_MAGIC_NUMBER1){
		ret_val = flexpth_cond_init_critical(cond);
		if(ret_val){
			// unable to initialized the mutex
			LOGERR("Unable to initialized flexpth cond\n");
			return EINVAL;
		}
	}

	/* release all waiters */
#if FLEXPTH_COND_DISTRIBUTE_LEVEL == 0
	/* fully distribute */
	fastsync_cond_broadcast((fastsync_cond*)cond->conds);
#elif FLEXPTH_COND_DISTRIBUTE_LEVEL == 1
	/* no distribute */
	fastsync_cond_broadcast((fastsync_cond*)cond->conds);
#else 
	/* static distribute */
	{
		int i;
		for(i = 0; i < FLEXPTH_COND_DISTRIBUTE_LEVEL; i++)
			fastsync_cond_broadcast((fastsync_cond*)(cond->conds) + 
						i);
	}
#endif


	return 0;
}

int flexpth_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *m,
			   const struct timespec *abs_timeout)
{
	// TODO: implement the mutex timed lock
	LOGERR("FLEXPTH cond timed wait not implemented\n");

	return EINVAL;
}

