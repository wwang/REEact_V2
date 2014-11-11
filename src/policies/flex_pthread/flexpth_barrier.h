/*
 * Header file for the flex-pthread barrier.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __FLEX_PTHREAD_BARRIER_H__
#define __FLEX_PTHREAD_BARRIER_H__

#include "flexpth_common_defs.h"

/*
 * data structure for tree-barrier attributes.
 */
struct flexpth_tree_barrier_attr{
	int dummy;
};

/*
 * data structure that holding all the per-thread function tree barriers. 
 * 
 * The reason of giving each thread function a copy of its own barrier:
 *
 * For the tree barrier to work, I need to know how many threads on a particular
 * core would use the barrier. Assume there is a barrier B, and two types of 
 * threads T1 and T2. The tree barrier requires me to know one a particular 
 * core, how many threads are using B. E.g., if there are T1 threads on core 1, 
 * I need to know if T1 are using B, so that I can assign correct barrier user 
 * number to core 1. However, since I do not have the source code, it is
 * impossible for me to know which of T1 or T2 uses barrier B (they way be both 
 * using B). So I create a tree for each barrier, and all these tree will have
 * one root node. The root node keeps the total number of threads that uses B, 
 * which is specified by the user when creating the barrier. The threads wait
 * at their own tree. Since these trees share one root, once there are enough
 * number of threads reaches the root, no matter what types these threads are,
 * these threads are released to proceed.
 *
 * The reason of separating the first barrier wait from the rest:
 * 
 * As aforementioned I need to know how many threads is running on a particular 
 * core. However, since only the user program is creating the threads, I do have
 * the information about how many threads are going to be created until all 
 * threads are actually created. Actually, I do not even know when all threads 
 * are created because I do not have the source code. This is not a problem for
 * non-treed barrier, since every thread has to wait at the root node. 
 * Fortunately, I can make this assumption safely: after the first wait of the
 * barrier, all threads using this barrier is already created. Because a barrier
 * wait can only succeed if all threads have waited on the barrier. Therefore, I
 * let the first wait using the root node, and the following wait on the actual
 * tree node.
 */
#define FLEXPTH_BARRIER_STATE_READY     0 // all threads using this bar created
#define FLEXPTH_BARRIER_STATE_NOT_READY 1 // not all threads created
#define FLEXPTH_BARRIER_STATE_INVALID   2 // the barrier is invalid
struct flexpth_tree_barrier{
	void *root; // root of the all tree, a fastsync_barrier object
	/* 
	 * 2D array of a tree barrier, index by thread function index, and
	 * the static linked list index. The _core_to_list_map can map a core id
	 * into corresponding static linked list index. As stated above, each
	 * function will have its own tree. Each element here is a 
	 * fastsync_barrier object
	 */
	void *func_tbars[FLEXPTH_MAX_THREAD_FUNCS][FLEX_PTHREAD_MAX_CORE_CNT*2];
	int func_cnt; // the number of thread functions
	int status; // whether the barrier is called the first time or is 
	            // invalid
};

/*
 * the structure for holding all barriers, this structure is saved into 
 * flex-pthread policy data
 */
struct flexpth_all_barriers{
	struct flexpth_tree_barrier tbars[FLEXPTH_MAX_BARRIERS];
	int tbar_cnt;
};

/*
 * Initialization function for flex_pthread barrier component.
 * flexpth_barrier_internal_init should be called with flexpth_init to
 * acquire information about the machine topology form REEact.
 *
 * Input Parameters:
 *      data: a pointer to REEact data (struct reeact_data *)
 * Return value:
 *      0: success
 *      1: wrong parameters
 *      2: memory allocation error
 */
int flexpth_barrier_internal_init(void *data);
int flexpth_barrier_internal_cleanup(void *data);


/*
 * Initialize a new flex_pthread tree barrier
 *
 * Input parameters:
 *     data: a pointer to REEact data
 *     attr: barrier attribute
 *     count: the number of threads using this barrier
 * Output parameters:
 *     tbar: the initialized tree barrier
 * Return values:
 *     0: success
 *     1: wrong parameter 
 *     2: flex-pthread barrier is not initialized
 *     3: Unable to allocate memory
 *     
 */
int flexpth_tree_barrier_init(void *data,
			      struct flexpth_tree_barrier **tbar,
			      struct flexpth_tree_barrier_attr *attr,
			      unsigned count);

/*
 * A new thread function is created, we need to add a tree for it for each
 * existing barrier
 *
 * Input parameters:
 *     data: a pointer to the REEact data
 *     finfo: the function information
 * Return values:
 *     0: success
 *     1: wrong parameter
 *     2: unable to allocate memory
 */
int flexpth_barrier_new_func(void *data, void *finfo);

/*
 * A new thread is created, update the barriers accordingly
 *
 * Input parameters:
 *     data: a pointer to the REEact data
 *     tinfo: the thread information
 * Return values:
 *     0: success
 *     1: wrong parameter
 */
int flexpth_barrier_new_thread(void *data, void *finfo);


#endif
