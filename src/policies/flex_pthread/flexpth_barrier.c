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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/futex.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <limits.h>

#include <common_toolx.h>
#include <simple_hashx.h>

#include "../../reeact.h"
#include "../../utils/reeact_utils.h"
#include "../../fastsync/fastsync.h"
#include "../reeact_policy.h"

#include "flex_pthread.h"
#include "flexpth_barrier.h"
#include "flexpth_thread_keeper.h"

/*
 * The static list that represents the tree structure of the tree-barrier,
 * which in-turn represents the machine topology. The value of each element
 * is the index of the parent of this element (SLIST_NULL means no parent).
 */
#define FLEXPTH_BAR_SLIST_NULL -1
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
int total_core_cnt = 0; // the total number of cores

/*
 * This internal initialization is responsible for creating the static linked
 * list and the two-way (core id <==> list) mapping.
 */
int flexpth_barrier_internal_init(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	struct processor_topo topo = rh->topology;
	int total_node_cnt = topo.socket_cnt * topo.node_cnt;
	int i,j,k,l;
	int node_id, core_id;

	total_core_cnt = topo.socket_cnt * topo.node_cnt * topo.core_cnt;
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
	_bar_slist.elements[0] = FLEXPTH_BAR_SLIST_NULL;
	for(i = 1; i < (total_node_cnt*2-1); i++){
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
	/*
	 * create the all tree barriers structure
	 */
	fh->barriers = malloc(sizeof(struct flexpth_all_barriers));
	if(fh->barriers == NULL){
		LOGERRX("Unable to allocate space for all barriers: ");
		return 2;
	}
	for(i = 0; i < FLEXPTH_MAX_BARRIERS; i++){
		((struct flexpth_all_barriers*)(fh->barriers))->tbars[i].status
			= FLEXPTH_BARRIER_STATE_INVALID;
	}
	       

	return 0;
}

int flexpth_barrier_internal_cleanup(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh;


	if(_core_to_list_map != NULL)
		free(_core_to_list_map);
	if(_list_to_core_map != NULL)
		free(_list_to_core_map);
	if(_bar_slist.elements != NULL)
		free(_bar_slist.elements);

	if(rh == NULL){
		LOGERR("wrong parameter, data is NULL\n");
		return 1;
	}

	fh = (struct flexpth_data*)rh->policy_data;

	if(fh != NULL && fh->barriers != NULL)
		free(fh->barriers);
	else{
		LOGERR("policy data (%p) and/or barriers (%p) is NULL\n", fh, 
		       fh->barriers);
		return 1;
	}
	fh->barriers = NULL;
	
	return 0;
}


/*
 * Initialize a new flex_pthread tree barrier per thread function
 *
 * Input parameters:
 *     tbar: the tree barrier to initialize
 *     attr: barrier attribute
 *     func: the function information
 * Return values:
 *     0: success
 *     1: tbar is NULL
 *     2: flex-pthread barrier is not initialized
 *     3: Unable to allocate memory
 *     
 */
int flexpth_tree_barrier_init_per_func(void **tbars,
				       struct flexpth_tree_barrier_attr *attr,
				       struct flexpth_thr_func_info * finfo)
{
	fastsync_barrier *barriers;
	int i;

	if(tbars == NULL || finfo == NULL){
		LOGERR("tbar (%p) or finfo (%p) is NULL\n", tbars, finfo);
		return 1;
	}

	if(_bar_slist.elements == NULL){
		LOGERR("flex-pthread barrier is not initialized");
		return 2;
	}
	
	/*
	 * allocate space for the fastsync_barriers
	 */
	barriers = (fastsync_barrier*)calloc(_bar_slist.len, 
					     sizeof(fastsync_barrier));
	if(barriers == NULL){
		LOGERRX("Unable to allocate space for barriers\n");
		return 3;
	}
	
	// set the parent pointers of the barriers
	for(i = 1; i < _bar_slist.len; i++){
		barriers[i].parent_bar = barriers + _bar_slist.elements[i];
		tbars[i] = (void*)(barriers + i);
	}
	// set parent of the root barrier to NULL and total_count to count
	tbars[0] = barriers;
	barriers[0].parent_bar = NULL;

	// set the thread count at each node
	for(i = 0; i < total_core_cnt; i++){
		int threads = finfo->thread_per_core[i];
		fastsync_barrier *tmp;
		barriers[_core_to_list_map[i]].total_count = threads;
		// update its parents
		tmp = barriers[_core_to_list_map[i]].parent_bar;
		while(tmp != NULL){
			tmp->total_count += threads;
			tmp = tmp->parent_bar;
		}
	}
       
#ifdef _REEACT_DEBUG_
	/*
	 * log the tree barrier array
	 */
	DPRINTF("Tree barrier created, array is:\n");
	for(i = 0; i < _bar_slist.len; i++){
		fprintf(stderr, "\t element %d (%p) parent %p count %d\n",
			i, barriers + i, barriers[i].parent_bar,
			barriers[i].total_count);
	}
	fprintf(stderr, "\n");
#endif
	
	return 0;
}

/*
 * initialized a tree barrier
 */
int flexpth_tree_barrier_init(void *data,
			      struct flexpth_tree_barrier **tbar,
			      struct flexpth_tree_barrier_attr *attr,
			      unsigned count)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh;
	struct flexpth_all_barriers *barriers;
	struct flexpth_thread_keeper *keeper;
	struct flexpth_thr_func_info *finfo;
	void *search_handle;
	int barrier_idx; // the index of current barrier
	fastsync_barrier *root;

	/*
	 * parameter checking
	 */
	if(rh == NULL || tbar == NULL){
		LOGERR("reeact data (%p) and/or tbar (%p) is NULL\n", rh, tbar);
		return 1;
	}
	fh = (struct flexpth_data*)rh->policy_data;
	if(fh == NULL){
		LOGERR("policy data (%p) is NULL\n", fh);
		return 1;
	}
	barriers = (struct flexpth_all_barriers*)fh->barriers;
	keeper = (struct flexpth_thread_keeper*)fh->thread_keeper;
	if(barriers == NULL || keeper == NULL){
		LOGERR("barriers (%p) or keeper (%p) is NULL\n", barriers, 
		       keeper);
		return 1;
	}

	/*
	 * get space for this barrier
	 */ 
	barrier_idx = barriers->tbar_cnt++;
	*tbar = barriers->tbars + barrier_idx;
	
	/*
	 * create the global root barrier
	 */
	(*tbar)->root = malloc(sizeof(fastsync_barrier));
	root = (fastsync_barrier*)(*tbar)->root;
	fastsync_barrier_init(root, NULL, count);
	(*tbar)->status = FLEXPTH_BARRIER_STATE_NOT_READY;
	(*tbar)->func_cnt = keeper->func_cnt;
	
	/*
	 * initialize each function's tree barrier
	 */
	search_handle = NULL;
	flexpth_keeper_get_next_func(data, &search_handle, &finfo);
	while(search_handle != NULL){
		fastsync_barrier *tmp;
		void **func_bar = (void**)((*tbar)->func_tbars + finfo->fidx);

		flexpth_tree_barrier_init_per_func(func_bar, attr, finfo);

		/*
		 * let the root of this tree point to the global root
		 */
		/* tmp = (fastsync_barrier*)((*tbar)->func_tbars[finfo->fidx][0]); */
		/* tmp->parent_bar = root; */
		/*
		 * given that I use binary tree, let the first two children in 
		 * in the binary directly associate with the global root to 
		 * save one level of barrier
		 */
		tmp = (fastsync_barrier*)((*tbar)->func_tbars[finfo->fidx][1]);
		tmp->parent_bar = root;
		tmp = (fastsync_barrier*)((*tbar)->func_tbars[finfo->fidx][2]);
		tmp->parent_bar = root;
		
		/*
		 * advance to next function
		 */
		flexpth_keeper_get_next_func(data, &search_handle, &finfo);
	}
	

	return 0;	
}

/*
 * A new thread function is created, we need to add a tree for it for each
 * existing barrier
 */
int flexpth_barrier_new_func(void *data, void *finfo_in)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh;
	struct flexpth_all_barriers *barriers;
	struct flexpth_tree_barrier *tbar;     
	struct flexpth_thr_func_info *finfo = finfo_in;
	fastsync_barrier *tmp;
	void **func_bar;

	int i;

	/*
	 * parameter checking
	 */
	if(rh == NULL || finfo == NULL){
		LOGERR("reeact data (%p) and/or tbar (%p) is NULL\n", rh, 
		       finfo);
		return 1;
	}
	fh = (struct flexpth_data*)rh->policy_data;
	if(fh == NULL){
		LOGERR("policy data (%p) is NULL\n", fh);
		return 1;
	}
	barriers = (struct flexpth_all_barriers*)fh->barriers;
	if(barriers == NULL){
		LOGERR("barriers is NULL\n");
		return 1;
	}

	/*
	 * update each barrier
	 */
	for(i = 0; i < barriers->tbar_cnt; i++){
		tbar = barriers->tbars + i;
		tbar->func_cnt++;
		func_bar = (void**)(tbar->func_tbars + finfo->fidx);

		flexpth_tree_barrier_init_per_func(func_bar, NULL, finfo);
		/*
		 * let the root of this tree point to the global root
		 */
		/* tmp = (fastsync_barrier*)(tbar->func_tbars[finfo->fidx][0]); */
		/* tmp->parent_bar = tbar->root; */
		/*
		 * given that I use binary tree, let the first two children in 
		 * in the binary directly associate with the global root to 
		 * save one level of barrier
		 */
		tmp = (fastsync_barrier*)(tbar->func_tbars[finfo->fidx][1]);
		tmp->parent_bar = tbar->root;
		tmp = (fastsync_barrier*)(tbar->func_tbars[finfo->fidx][2]);
		tmp->parent_bar = tbar->root;
	}

	return 0;
}

/*
 * A new thread is created, update the barriers accordingly
 */
int flexpth_barrier_new_thread(void *data, void *tinfo_in)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh;
	struct flexpth_all_barriers *barriers;
	struct flexpth_tree_barrier *tbar;     
	struct flexpth_thread_info *tinfo = tinfo_in;
	fastsync_barrier *tmp;
	int i;

	/*
	 * parameter checking
	 */
	if(rh == NULL || tinfo == NULL){
		LOGERR("reeact data (%p) and/or tbar (%p) is NULL\n", rh, 
		       tinfo);
		return 1;
	}
	fh = (struct flexpth_data*)rh->policy_data;
	if(fh == NULL){
		LOGERR("policy data (%p) is NULL\n", fh);
		return 1;
	}
	barriers = (struct flexpth_all_barriers*)fh->barriers;
	if(barriers == NULL){
		LOGERR("barriers is NULL\n");
		return 1;
	}

	/*
	 * update each barrier
	 */
	for(i = 0; i < barriers->tbar_cnt; i++){
		/*
		 * find the correct barrier for this thread's function and core
		 */
		tbar = barriers->tbars + i;
		tmp = (fastsync_barrier*)
			tbar->func_tbars[tinfo->fidx]
			[_core_to_list_map[tinfo->core_id]];
		/*
		 * update the total count of this barrier and its ancestors; 
		 * note that, the count of the global root should not be changed
		 */
		while(tmp->parent_bar != NULL){
			tmp->total_count++;
			tmp = tmp->parent_bar;
		}		
	}

	return 0;
}

/*
 * first barrier synchronization of a tree barrier. Because all of the threads, 
 * that are using this barrier may not be ready yet, we need to wait at the
 * root barrier. This is the same as a normal barrier wait, except after all
 * threads has reached the barrier, the status of the tree barrier is set to
 * ready. 	 
 * For the sake of performance, I do not test parameter here.
 */
int flexpth_barrier_first_wait(struct flexpth_tree_barrier *tbar)
{
	int ret_val = 1;
	fastsync_barrier *barrier = (fastsync_barrier*)tbar->root;
	int cur_seq = atomic_read(barrier->seq);
	// atomic add and fetch
	int count = atomic_add(&(barrier->waiting), 1);
	
	// done waiting for the barrier
	if(count == barrier->total_count){
		/* 
		 * this is the last thread hitting the barrier, so set the tree
		 * barrier to ready. This operation is safe since only one (the
		 * last) thread can reach here.
		 */
		tbar->status = FLEXPTH_BARRIER_STATE_READY;

#ifdef _REEACT_DEBUG_
		{
			int i,j;
			fastsync_barrier *bar;
			/*
			 * log the tree barrier array
			 */
			DPRINTF("Tree barrier READY, array is:\n");
			bar = (fastsync_barrier*)tbar->root;
			DPRINTF("\t root node (%p) count %d\n", bar, bar->total_count);
			for(i = 0; i < tbar->func_cnt; i++){
				printf("\t thread function %d\n", i);
				for(j = 0; j < _bar_slist.len; j++){
					bar = (fastsync_barrier*)tbar->func_tbars[i][j];
					fprintf(stderr, "\t\t node %d (%p) "
						"on core %d, parent %p, count %d\n",
						j, bar, _list_to_core_map[j],
						bar->parent_bar,
						bar->total_count);
				}
			}
			fprintf(stderr, "\n");
		}
#endif

		/*
		 * clear the waiting count and increment sequence count
		 * these operations should be done simultaneously 
		 */
		gcc_barrier();
		barrier->reset = barrier->seq + 1;
		gcc_barrier();
#ifdef _FUTEX_BARRIER_
		
		/*
		 * wake up other threads waiting at this barrier
		 */
		if(barrier->total_count > 1)
			sys_futex(&barrier->seq, FUTEX_WAKE_PRIVATE,
				  INT_MAX, NULL, NULL, 0);
#endif
		ret_val = PTHREAD_BARRIER_SERIAL_THREAD;
		return ret_val;
	}

	if(count < barrier->total_count){
		while (cur_seq == atomic_read(barrier->seq)) {
#ifndef _FUTEX_BARRIER_
			sched_yield(); // give up processor
#else
			// block thread itself
			sys_futex(&barrier->seq, FUTEX_WAIT_PRIVATE,
				  cur_seq, NULL, NULL, 0);
#endif
		}
		// normal wait success
		ret_val = 0;
		//break;
		return ret_val;
	}
	
	/* should be unreachable */
	return ret_val;
	
}

/*
 * pthread hooks
 */
int flexpth_barrier_destroy(pthread_barrier_t *barrier)
{
	struct flexpth_tree_barrier *tbar;
	
	if(barrier == NULL){
		LOGERR("destroying NULL barrier\n");
		return EINVAL;
	}
	
	tbar = *(struct flexpth_tree_barrier**)barrier;

	/*
	 * I do not actually free the barrier, just mark it invalid
	 */
	if(tbar != NULL)
		tbar->status = FLEXPTH_BARRIER_STATE_INVALID;
	
	return 0;
}

extern struct reeact_data *reeact_handle; // the global REEact data handle
int flexpth_barrier_init(pthread_barrier_t *barrier,
			 const pthread_barrierattr_t *attr, unsigned count)
{
	struct flexpth_tree_barrier *tbar;
	int ret_val;

	if(barrier == NULL || count == 0)
		return EINVAL;

	/* 
	 * create a new flex-pthread tree barrier and save the pointer
	 * pthread_barrier_t *barrier
	 */
	ret_val = flexpth_tree_barrier_init((void*)reeact_handle, &tbar, NULL, 
					    count);
	if(ret_val != 0)
		/*
		 * some memory errors
		 */
		return ENOMEM;

	*((struct flexpth_tree_barrier**)barrier) = tbar;
	
	return 0;

}

extern __thread struct flexpth_thread_info* self; // information of this thread
extern __thread long long barrier_idx; 
int flexpth_barrier_wait(pthread_barrier_t *barrier)
{	
	struct flexpth_tree_barrier *tbar;
	fastsync_barrier *bar;
	int fidx, core_id;
		
	if(barrier == NULL)
		return EINVAL;

	tbar = *(struct flexpth_tree_barrier**)barrier;

	switch(tbar->status){
	case FLEXPTH_BARRIER_STATE_INVALID:
		/*
		 * barrier does not exist
		 */
		return EINVAL;
	case FLEXPTH_BARRIER_STATE_NOT_READY:
		/*
		 * the barrier is called for the first time
		 */
		return flexpth_barrier_first_wait(tbar);
	case FLEXPTH_BARRIER_STATE_READY:
		/*
		 * the barrier is ready with all threads created
		 */
		/* bar = (fastsync_barrier*)tbar->func_tbars[self->fidx] */
		/* 	[_core_to_list_map[self->core_id]]; */
		fidx = barrier_idx >> 32;
		core_id = barrier_idx & 0x00000000ffffffff;
		bar = (fastsync_barrier*)tbar->func_tbars[fidx]
			[_core_to_list_map[core_id]];
		return fastsync_barrier_wait(bar);
	default:
		/*
		 * barrier has unknown status
		 */
		return EINVAL;
	}
	
	/* unreachable */
}
