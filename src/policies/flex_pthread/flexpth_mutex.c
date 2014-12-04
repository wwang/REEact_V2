/*
 * Mutex implementation for flex-pthread. Flex-pthread mutex is essentially a 
 * wrapper to the fastsync mutex. The only trick here is that a pthread mutex is
 * initialized to be a fastsync mutex the first time the mutex is being locked.
 *
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
			    void **mutex,
			    struct flexpth_tree_mutex_attr *attr)
{
	struct reeact_data *rh = (struct reeact_data*)data;

	/*
	 * parameter checking
	 */
	if(rh == NULL || mutex == NULL){
		LOGERR("reeact data (%p) and/or tmutex (%p) is NULL\n", rh,
		       mutex);
		return 1;
	}

	/*
	 * allocate space for the fastsync_mutex
	 */ 
	*mutex = calloc(1, sizeof(fastsync_mutex));
	if(*mutex == NULL){
		LOGERRX("Unable to allocate space for flex-pthread mutex: ");
		return 2;
	}
	
	/*
	 * initialized the mutex
	 */
	fastsync_mutex_init((fastsync_mutex*)*mutex, NULL);
	
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
					  &(mutex->mutex), NULL);

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
 * number during this gap, then we are in trouble. Although this should never
 * happen in correct codes.
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
					&(mutex->mutex), NULL);
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
	
	/* lock the mutex */
	return fastsync_mutex_lock((fastsync_mutex*)mutex->mutex);
}


int flexpth_mutex_unlock(pthread_mutex_t *m)
{
	struct flexpth_mutex *mutex = (struct flexpth_mutex*)m;

	if(mutex == NULL)
		return EINVAL;

	if(mutex->magic_number != FLEXPTH_MUTEX_MAGIC_NUMBER1)
		// unlocking a non-fastsync or uninitialized mutex
		return 0;

	/* lock the mutex */
	return fastsync_mutex_unlock((fastsync_mutex*)mutex->mutex);
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
