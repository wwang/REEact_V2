/*
 * Implementation of thread creation function for flex-pthread
 *
 * Author: Wei Wang <wwang@virginia.edu>
 *
 */

#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <common_toolx.h>

#include "../../reeact.h"
#include "../../utils/reeact_utils.h"
#include "../../pthread_hooks/pthread_hooks_originals.h"

#include "flex_pthread.h"
#include "flexpth_thread_keeper.h"
#include "flexpth_env_var.h"

/* 
 * Global REEact data 
 */
extern struct reeact_data *reeact_handle;

/*
 * thread function type
 */
typedef void* (*thread_function)(void *arg);

/*
 * thread id of current running thread. I used thread local storage for the 
 * sake of readability and portability.
 * Because I only need a way to uniquely identify a thread, a faster way would
 * be relying on the gs or fs segment register
 */
__thread struct flexpth_thread_info* self;
/*
 * 64-bit integer, first half is the fidx, second hard is core_id
 */
__thread long long barrier_idx; 


/*
 * New threads are created to run this wrapper function first. This wrapper
 * function then calls the application's thread function
 */
void * flexpth_thread_wrapper(void *arg)
{
	cpu_set_t cores;
	int ret_val;
	void *thread_return;
	thread_function start_routine;
	struct sched_param sched_param = {0};

	self = (struct flexpth_thread_info*)arg;

	if(self == NULL){
		LOGERR("new thread created with NULL thread info\n");
		return NULL;
	}

	/*
	 * get thread id
	 */
	self->tid = gettid();
	
	DPRINTF("%d'th thread (tid %d) started (%p) on core %d\n", 
		self->tidx, self->tid, self->func, self->core_id);
	
	/*
	 * pin thread to core
	 */
	CPU_ZERO(&cores);
	CPU_SET(self->core_id, &cores);
	ret_val = sched_setaffinity(self->tid, sizeof(cpu_set_t), &cores);
	if(ret_val != 0)
		LOGERRX("Unable to pin %d'th thread (%d) to core %d\n", 
			self->tidx, self->tid, self->core_id);
	/*
	 * set to use batch scheduling to reduce context switch frequencies
	 */
	if(sched_setscheduler(0, SCHED_BATCH, &sched_param)){
		LOGERRX("thread %d failed to set sched_batch: ", self->tidx);
	}


	/*
	 * set the barrier index thread local storage
	 */
	barrier_idx = (((long long)self->fidx) << 32) | self->core_id;	
		
	/*
	 * execute the real thread function
	 */ 
	start_routine = (thread_function) self->func;
	thread_return = start_routine(self->arg);
	
	
	/*
	 * thread finished
	 */
	DPRINTF("%d'th thread (tid %d) finished\n", self->tidx, self->tid);
	flexpth_keeper_remove_thread((void*)reeact_handle, self->tidx, self);

	/*
	 * TODO: remove thread from thread keeper, make thread keeper thread-safe
	 */
	
	return thread_return;				   
}

/*
 * find a core to assign. This is a temporary testing function
 */
int core_counter = 0;
static int assign_core(struct flexpth_core_list *cl)
{
	int core_id;
	core_id = cl->cores[core_counter++];
	if(core_counter >= cl->core_cnt)
		core_counter = 0;
	return core_id;
}

/*
 * create a new pthread
 */
int flexpth_create_thread(pthread_t *thread, pthread_attr_t *attr,
			  void *(*start_routine)(void *), void *arg)
{
	struct reeact_data *rh = reeact_handle;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	int ret_val;
	struct flexpth_thread_info *tinfo;
	int core_id;
	
	/*
	 * add thread to thread info
	 */
	core_id = assign_core((struct flexpth_core_list*)fh->core_list);
	ret_val = flexpth_keeper_add_thread((void*)rh, core_id, 
					    (void*)start_routine, &tinfo);

	if(ret_val != 0){
		LOGERR("failed to add new thread to thread keeper (%d)\n",
		       ret_val);
		return EAGAIN;
	}

	/*
	 * special handling for main thread; if the main thread uses the 
	 * same thread function as the worker threads, update the
	 * thread function of the main thread
	 */ 
	if(fh->control_main_thr == 1){
		DPRINTF("update main thread function to 0x%08x\n", start_routine);
		ret_val = flexpth_keeper_update_thread_func(rh, 0,
							    start_routine);
		if(ret_val != 0)
			LOGERR("failed to update main thread function\n");
		
		fh->control_main_thr = 2;
	}
	
	/*
	 * create the new thread
	 */
	tinfo->arg = arg;
	return real_pthread_create(thread, attr, flexpth_thread_wrapper,
				   (void*)tinfo);
	/* return real_pthread_create(thread, attr, start_routine, arg); */
}


/*
 * put main thread under control
 */
int flexpth_control_main_thr(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh;
	cpu_set_t cores;
	struct flexpth_thread_info *tinfo;
	int core_id;
	int ret_val;

	if(rh == NULL)
		return 1;
	fh = (struct flexpth_data*)rh->policy_data;

	if(fh == NULL || !fh->control_main_thr)
		return 1;

	/*
	 * add thread to thread info
	 */
	core_id = assign_core((struct flexpth_core_list*)fh->core_list);
	if(fh->control_main_thr != 1){
		/*
		 * if fh->control_main_thr is not 1, then we use
		 * fh->control_main_thr as main thread's thread function address
		 */
		DPRINTF("add main thread with thread function 0x%08x\n", 
			fh->control_main_thr);
		ret_val = flexpth_keeper_add_thread((void*)rh, core_id, 
						    (void*)fh->control_main_thr,
						    &tinfo);
	}
	else{
		/*
		 * if fh->control_main_thr is 1, worker thread function is used 
		 * as main thread function. Therefore, I have to postpone adding
		 * main thread's thread function information
		 */
		DPRINTF("add main thread with no thread function\n");
		ret_val = flexpth_keeper_add_thread_nofunc((void*)rh, core_id,
							   &tinfo);
	}
	self = tinfo;

	if(ret_val != 0){
		LOGERR("failed to add main thread to thread keeper (%d)\n",
		       ret_val);
		return 3;
	}

	/*
	 * get thread id
	 */
	self->tid = gettid();

	/*
	 * pin thread to core
	 */
	CPU_ZERO(&cores);
	CPU_SET(self->core_id, &cores);
	ret_val = sched_setaffinity(self->tid, sizeof(cpu_set_t), &cores);
	if(ret_val != 0){
		LOGERRX("Unable to pin main thread (%d) to core %d\n", 
			self->tid, core_id);
		return 2;
	}

	/*
	 * set the barrier index thread local storage; no matter how main thread
	 * is handled, its thread function will always be the first one, i.e, 
	 * fidx is always 0, so there is no need to change barrier_idx after 
	 * setting it here.
	 */
	barrier_idx = (((long long)self->fidx) << 32) | self->core_id;	
	
	return 0;
}

