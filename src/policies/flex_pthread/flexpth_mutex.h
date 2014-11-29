/*
 * Header file for the flex-pthread mutex.
 *
 * Author: Wei Wang <wwang@virginia.ed>
 */

#ifndef __FLEX_PTHREAD_BARRIER_H__
#define __FLEX_PTHREAD_BARRIER_H__

#include "flexpth_common_defs.h"

/*
 * data structure for the tree-mutex attributes
 */
struct flexpth_tree_mutex_attr{
	// TODO: add more attributes
	int dummy;
};

/*
 * data structure that holds the information of the mutex tree
 */

struct flexpth_tree_mutex{
	/* array of pointers to fastsync_mutex */
	void *mutexes[FLEX_PTHREAD_MAX_CORE_CNT*2]; 
};

/*
 * data structure that holds the information of one flex-pthread mutex
 * this data structure will be used to replace the content of pthread_mutex_t.
 * Note pthread_mutex_t is 40 bytes at the moment.
 */
struct flexpth_mutex{
	/*
	 * The magic number that indicates this mutex if a flex-pthread mutex. 
	 * Because users can initialize a pthread mutex without calling the
	 * pthread_mutex_init function, I have to initialize a mutex on the 
	 * first call of mutex_lock. For the following calls, the mutex can
	 * be safely used. To determine whether the mutex has been properly
	 * initialized, I introduced this magic number.
	 */
	int magic_number;
	void *pth_mutex; // stores the old pthread mutex information 
	struct flexpth_tree_mutex *tmutex; // the tree mutex
};

/*
 * Initialization function for flex_pthread mutex component. For the tree
 * barrier to work, the processor topology is required. However, since the 
 * topology is already acquired with flexpth_barrier_internal_init, there is 
 * nothing more to do for mutex_internal_init at the moment.
 *
 * Input Parameters:
 *      data: a pointer to REEact data (struct reeact_data *)
 * Return value:
 *      0: success
 *      1: wrong parameters
 *      2: memory allocation error
 */
int flexpth_mutex_internal_init(void *data);
int flexpth_mutex_internal_cleanup(void *data);


/*
 * Initialize a new flex_pthread tree mutex
 *
 * Input parameters:
 *     data: a pointer to REEact data
 *     attr: mutex attributes
 * Output parameters:
 *     tmutex: the initialized tree barrier
 * Return values:
 *     0: success
 *     1: wrong parameter 
 *     2: topology information not ready
 *     3: Unable to allocate memory
 *     
 */
int flexpth_tree_mutex_init(void *data,
			    struct flexpth_tree_mutex **tmutex,
			    struct flexpth_tree_mutex_attr *attr);


#endif
