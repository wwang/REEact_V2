/*
 * Header file for the flex-pthread barrier.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __FLEX_PTHREAD_BARRIER_H__
#define __FLEX_PTHREAD_BARRIER_H__

/*
 * data structure for tree-barrier. Barriers are stored in an array
 * simulating static linked list. 
 */
struct flexpth_tree_barrier{
	void * barriers; // the barrier array; here each element is a 
                         // fastsync_barrier
	int barrier_cnt; // the number of barrier in the array
};

/*
 * data structure for tree-barrier attributes.
 */
struct flexpth_tree_barrier_attr{
	int dummy;
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
 */
int flexpth_barrier_internal_init(void *data);
int flexpth_barrier_internal_cleanup(void *data);


/*
 * Initialize a new flex_pthread tree barrier
 *
 * Input parameters:
 *     tbar: the tree barrier to initialize
 *     attr: barrier attribute
 *     count: the number of threads using this barrier
 * Return values:
 *     0: success
 *     1: tbar is NULL
 *     2: flex-pthread barrier is not initialized
 *     3: Unable to allocate memory
 *     
 */
int flexpth_tree_barrier_init(struct flexpth_tree_barrier *tbar,
			      struct flexpth_tree_barrier_attr *attr,
			      unsigned count);
#endif
