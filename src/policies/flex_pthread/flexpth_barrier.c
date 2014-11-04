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

#include <stdio.h>
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
	int *elements;
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
	int total_core_cnt = topo.socket_cnt * topo.node_cnt * topo.core_cnt;
	int total_node_cnt = topo.socket_cnt * topo.node_cnt;
	int i,j,k,l;
	int node_id, core_id;

	/*
	 * determine how many tree-leaf in the tree. The leaf nodes represents
	 * therefore the number of leaves equals to the number of cores. For
	 * none leave nodes, I used a binary tree structure. I assume that the
	 * numbers of sockets and nodes are power of 2, which is NOT always 
	 * true. Given C cores and N nodes, there are 2*N-1 non-leaves, and C
	 * leaves
	 */
	_bar_slist.len = total_node_cnt * 2 - 1 + total_core_cnt;
	_bar_slist.elements = (int*)calloc(_bar_slist.len, sizeof(int));
	
	/*
	 * generate the parent index for each list element. For non leaf node
	 * i, where it is a node in a binary tree, its parent is floor((i-1)/2)
	 * For leaf node i, its parent is: (let N be the number of nodes)
	 * floor((i - (2 * N - 1)) / topo.core_cnt) + N - 1
	 */
	for(i = 0; i < (total_node_cnt*2-1); i++){
		// for non-leaf nodes
		_bar_slist.elements[i] = (i-1)/2;
	}
	for(i = total_node_cnt * 2 -1; i < _bar_slist.len; i++){
		// for leaf nodes
		_bar_slist.elements[i] = (i - 2 * total_node_cnt + 1) / 
			topo.core_cnt + total_node_cnt - 1;
	}
	
#ifdef _REEACT_DEBUG_
	/*
	 * log the static tree
	 */
	DPRINTF("Static linked list (%d): ", _bar_slist.len);
	for(i = 0; i < _bar_slist.len; i++)
	        fprintf(stderr, "%d ", _bar_slist.elements[i]);
	fprintf(stderr, "\n");
#endif
	/*
	 * generate the two-way mapping of core id to element index
	 */
	_core_to_list_map = (int*)calloc(total_core_cnt, sizeof(int));
	_list_to_core_map = (int*)calloc(_bar_slist.len, sizeof(int));
	l = 2 * total_node_cnt - 1;
	// assign the element to cores based on socket and node clustering
	for(i = 0; i < topo.socket_cnt; i++){
		for(j = 0; j < topo.node_cnt; j++){
			node_id = topo.nodes[i * topo.node_cnt +j];
			for(k = 0; k < topo.core_cnt; k++){
				core_id = topo.cores[node_id * 
						     topo.core_cnt + k];
				_core_to_list_map[core_id] = l;
				_list_to_core_map[l] = core_id;
				l++;
			}
		}
	}
#ifdef _REEACT_DEBUG_
	/*
	 * log the core mapping
	 */
	DPRINTF("Core to list mapping: ");
	for(i = 0; i < total_core_cnt; i++)
	        fprintf(stderr, "%d=>%d,", i, _core_to_list_map[i]);
	fprintf(stderr, "\n");
	DPRINTF("list to core mapping: ");
	for(i = 2 * total_node_cnt - 1; i < _bar_slist.len; i++)
	        fprintf(stderr, "%d=>%d,", i, _list_to_core_map[i]);
	fprintf(stderr, "\n");
	
#endif

	return 0;
}

int flexpth_barrier_internal_cleanup(void *data)
{
	if(_core_to_list_map != NULL)
		free(_core_to_list_map);
	if(_list_to_core_map != NULL)
		free(_list_to_core_map);
	if(_bar_slist.elements != NULL)
		free(_bar_slist.elements);
	
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
