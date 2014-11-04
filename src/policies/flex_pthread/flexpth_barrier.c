/*
 * Barrier implementation for flex-pthread.
 *
 * Notes on tree-barrier implementation:
 * 1. I call a tree-node in the barrier-tree a sub-barrier
 * 2. The tree should reflect the processor topology of the NUMA architecture, 
 *    for the sake of best performance. Example, a 4-node 1-socket machine,
 *    Machine-level-barrier
 *        |--node-level-barrier
 *        |      |--core-level-barrier
 *        |      |--core-level-barrier
 *        |--node-level-barrier
 *        |      |--core-level-barrier
 *        |      |--core-level-barrier
 *    Note that other tree structures are still possible, the code here is
 *    generic.
 * 3. if a sub-barrier represents a core, it should be a leaf of the tree, 
 *    and I use blocked-wait on it
 * 4. if a sub-barrier represents a node or higher level, it should be a non-
 *    leaf node, and I use spinning (busy wait) on it
 * 5. The tree is stored in an array, representing a static linked list. I use
 *    static linked list because the tree structure will not change on a 
 *    machine (since it reflects the machine topology), and there is a need for
 *    creating many tree-barrier instances. With static linked list, I can 
 *    simply create a template of the tree barrier before hand. When creating
 *    a new tree-barrier, all I have to do is a memcpy on the template.
 * Author: Wei Wang <wwang@virginia.edu>
 */


#include <stdlib.h>

#include "../../reeact.h"
#include "../../utils/reeact_utils.h"
#include "../../fastsync/fastsync.h"
#include "../reeact_policy.h"

/*
 * The static list that represents the tree structure of the tree-barrier,
 * which in-turn represents the machine topology. The value of each element
 * is the index of the parent of this element
 */
struct _flexpth_bar_slist{
	int *element;
	int len;
};
struct _flexpth_bar_slist _bar_slist = {0};

/*
 * two-way mapping that maps a core id to its corresponding element in the
 * static list and vice-versa.
 */
int *_core_to_list_map = NULL; // core to list element mapping
int *_list_to_core_map = NULL; // list element to core mapping

/*
 * This internal initialization is responsible for creating the static linked
 * list and the two-way (core id <==> list) mapping.
 */
int flexpth_barrier_internal_init(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct processor_topo topo = rh->topology;

	
	
}

int flexpth_barrier_internal_cleanup(void *data)
{
	if(_core_to_list_map != NULL)
		free(core_to_list_map);
	if(_list_to_core_map != NULL)
		free(core_to_list_map);
	if(_bar_slist.element != NULL)
		free(_bar_slist.element);
	
	return 0;
}

int flexpth_barrier_wait(pthread_barrier_t *barrier)
{	
	fastsync_barrier *bar = *(fastsync_barrier**) barrier;

	return fastsync_barrier_wait(bar);
}

int flexpth_barrier_destroy(pthread_barrier_t *barrier)
{
	int ret_val;

	fastsync_barrier *bar = *(fastsync_barrier**) barrier;
	
	ret_val = fastsync_barrier_destroy(bar);
	
	if(bar != NULL)
		free(bar);
	
	return ret_val;
}
int flexpth_barrier_init(pthread_barrier_t *barrier,
			 const pthread_barrierattr_t *attr, unsigned count)
{
	/* 
	 * create a new fast sync barrier object and save it in the
	 * pthread_barrier_t *barrier
	 */
	fastsync_barrier *bar = malloc(sizeof(fastsync_barrier));
	*((fastsync_barrier**)barrier) = bar;
	
	// initialize fast barrier
	return fastsync_barrier_init(bar, NULL, count);
}
