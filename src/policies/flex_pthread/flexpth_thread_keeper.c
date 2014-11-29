/*
 * Implementation of the thread tracking/bookkeeping component for flex-pthread.
 * This component keeps information about thread functions, threads and their 
 * core allocations.
 * 
 * Note that these implementation are not thread safe for at the moment. Not 
 * sure if thread-safe is necessary. Need a reader-writer lock to make this 
 * thread-safe.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdlib.h>

#include <simple_hashx.h>

#include "../../reeact.h"
#include "../../utils/reeact_utils.h"

#include "flexpth_common_defs.h"
#include "flex_pthread.h"
#include "flexpth_thread_keeper.h"
#include "flexpth_barrier.h"


#define FLEXPTH_THREAD_TABLE_LEN  1024
#define FLEXPTH_FUNC_TABLE_LEN 16


/*
 * a counter for assigned thread and function index
 */
int cur_tidx = 0;
int cur_fidx = 0;

/*
 * Initialized the thread keeper component
 */
int flexpth_thread_keeper_init(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	struct flexpth_thread_keeper *k;
	int ret_val;

	if(data == NULL){
		LOGERR("input thread data is NULL\n");
		return 1;
	}
	
	/*
	 * allocate space for thread keeper
	 */
	fh->thread_keeper = malloc(sizeof(struct flexpth_thread_keeper));
	k = (struct flexpth_thread_keeper*)fh->thread_keeper;
	if(k == NULL){
		LOGERRX("Unable to allocate memory for thread keeper\n");
		return 2;
	}

	/*
	 * allocate space for threads hash table and funcs hash table
	 */	
	ret_val = initialize_simple_hashx(&(k->threads), 
					  FLEXPTH_THREAD_TABLE_LEN);
	if(ret_val != 0){
		LOGERRX("Unable to create thread hash table, error %d, "
			"system error: \n", ret_val);
		return 2;
	}

	ret_val = initialize_simple_hashx(&(k->funcs), 
					  FLEXPTH_FUNC_TABLE_LEN);
	if(ret_val != 0){
		LOGERRX("Unable to create thread function hash table, error %d,"
			" system error: \n", ret_val);
		return 2;
	}
	
	k->thread_cnt = 0;
	k->func_cnt = 0;

	return 0;
}

/*
 * Clean up the thread keeper component
 */
int flexpth_thread_keeper_cleanup(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	struct flexpth_thread_keeper *k;

	if(data == NULL){
		LOGERR("input thread data is NULL\n");
		return 1;
	}
	
	k = (struct flexpth_thread_keeper*)fh->thread_keeper;
	if(k == NULL){
		LOGERRX("thread keeper is NULL\n");
		return 1;
	}
	
	if(k->threads != NULL)
		cleanup_simple_hashx(k->threads);
	if(k->funcs != NULL)
		cleanup_simple_hashx(k->funcs);

	free(k);
	
	fh->thread_keeper = NULL;
	
	return 0;
}

/*
 * Add the information of a new thread
 */
int flexpth_keeper_add_thread(void *data, int core_id, void* func,
			      struct flexpth_thread_info **tinfo_out)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	struct flexpth_thread_keeper *k;
	int ret_val;
	struct flexpth_thread_info *tinfo;
	struct flexpth_thr_func_info *finfo;

	if(data == NULL || tinfo_out == NULL){
		LOGERR("wrong parameter: data (%p) and tinfo (%p)\n",
		       data, tinfo_out);
		return 1;
	}
	
	k = (struct flexpth_thread_keeper*)fh->thread_keeper;
	if(k == NULL || k->threads == NULL  || k->funcs == NULL){
		LOGERRX("thread keeper (%p) or thread table (%p) or "
			"function table (%p) is NULL\n", k, k->threads, 
			k->funcs);
		return 1;
	}

	// locate the function record for func
	ret_val = get_val_simple_hashx(k->funcs, (long long)func, 1, NULL,
				       (void**)&finfo);
	if(ret_val == 2){
		// this is the first thread for this function
		finfo = (struct flexpth_thr_func_info*)
			calloc(1, sizeof(struct flexpth_thr_func_info));
		if(finfo == NULL){
			LOGERRX("unable allocate space for function info\n");
			return 2;
		}
		finfo->func = func;
		finfo->fidx = cur_fidx++;
		save_val_simple_hashx(k->funcs, (long long)func, 1, 0, 
				      (void*)finfo);
		k->func_cnt++;
		// update barriers
		flexpth_barrier_new_func(data, (void *)finfo);
	}

	// create a new record for the thread
	*tinfo_out = tinfo = (struct flexpth_thread_info*)
		calloc(1, sizeof(struct flexpth_thread_info));
	if(tinfo == NULL){
		LOGERRX("unable allocate space for thread info\n");
		return 2;
	}
	tinfo->tidx = cur_tidx++;
	tinfo->core_id = core_id;
	tinfo->func = func;
	tinfo->fidx = finfo->fidx;
	save_val_simple_hashx(k->threads, tinfo->tidx, 1, 0, (void*)tinfo);
	k->thread_cnt++;

	// add thread information to function information
	finfo->thread_cnt++;
	finfo->thread_per_core[core_id]++;

	// update barriers
	flexpth_barrier_new_thread(data, (void *)tinfo);

	return 0;
}

/*
 * Remove the information of a thread
 */
int flexpth_keeper_remove_thread(void *data, int tidx,
				 struct flexpth_thread_info *tinfo_in)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	struct flexpth_thread_keeper *k;
	int ret_val;
	struct flexpth_thread_info *tinfo;
	struct flexpth_thr_func_info *finfo;
	void *func;

	if(data == NULL){
		LOGERR("input thread data is NULL\n");
		return 1;
	}	

	k = (struct flexpth_thread_keeper*)fh->thread_keeper;
	if(k == NULL || k->threads == NULL  || k->funcs == NULL){
		LOGERRX("thread keeper (%p) or thread table (%p) or "
			"function table (%p) is NULL\n", k, k->threads, 
			k->funcs);
		return 1;
	}

	// locate the thread record for 
	if(tinfo_in == NULL){
		ret_val = get_val_simple_hashx(k->threads, tidx, 1, NULL, 
					       (void**)&tinfo);
		if(ret_val == 2){
			// thread does not exists
			LOGERR("thread %d not exists\n", tidx);
			return 2;
		}
	}
	else
		tinfo = tinfo_in;
	
	func = tinfo->func;
	// locate the function record for 
	ret_val = get_val_simple_hashx(k->funcs, (long long)func, 1, NULL, 
				       (void**)&finfo);
	if(ret_val == 2){
		// function does not exists
		LOGERR("function %p of thread %d not exists\n", func, tidx);
		return 3;
	}

	// remove thread from function info
        atomic_subf(&finfo->thread_cnt, -1);
	atomic_subf(finfo->thread_per_core + tinfo->core_id, -1);

	
	// TODO: remove thread form thread table
	/* remove_val_simple_hashx(k->threads, tidx); */
	/* k->thread_cnt--; */
      

	return 0;
}

/*
 * Change the running core of a thread
 */
int flexpth_keeper_thread_migrate(void *data, int tidx, int core_id)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	struct flexpth_thread_keeper *k;
	int ret_val;
	struct flexpth_thread_info *tinfo;
	struct flexpth_thr_func_info *finfo;
	void *func;
	int old_core;

	if(data == NULL){
		LOGERR("input thread data is NULL\n");
		return 1;
	}
	
	k = (struct flexpth_thread_keeper*)fh->thread_keeper;
	if(k == NULL || k->threads == NULL  || k->funcs == NULL){
		LOGERRX("thread keeper (%p) or thread table (%p) or "
			"function table (%p) is NULL\n", k, k->threads, 
			k->funcs);
		return 1;
	}

	// locate the thread record for 
	ret_val = get_val_simple_hashx(k->threads, tidx, 1, NULL, 
				       (void**)&tinfo);
	if(ret_val == 2){
		// thread does not exists
		LOGERR("thread %d not exists\n", tidx);
		return 2;
	}
	//update thread info
	old_core = tinfo->core_id;
	tinfo->core_id = core_id;
	
	
	func = tinfo->func;
	// locate the function record for 
	ret_val = get_val_simple_hashx(k->funcs, (long long)func, 1, NULL, 
				       (void**)&finfo);
	if(ret_val == 2){
		// function does not exists
		LOGERR("function %p of thread %d not exists\n", func, tidx);
		return 3;
	}

	// update function info
	finfo->thread_per_core[old_core]--;
	finfo->thread_per_core[core_id]++;

	return 0;
}

/*
 * Get the next function's information
 */
int flexpth_keeper_get_next_func(void *data, void **search_handle, 
				 struct flexpth_thr_func_info **finfo)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh;
	struct flexpth_thread_keeper *keeper;
	
	if(data == NULL || search_handle == NULL || finfo == NULL){
		LOGERR("wrong parameters: data (%p), search_handle (%p) and "
		       "finfo (%p)\n", data, search_handle, finfo);
		return 1;
	}
	fh = (struct flexpth_data*)rh->policy_data;
	keeper = (struct flexpth_thread_keeper*)fh->thread_keeper;
	
	get_next_simple_hashx(keeper->funcs, 1, search_handle, NULL, 
			      (void**)finfo);

	return 0;
}

