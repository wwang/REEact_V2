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
			self->tidx, self->tid);

	/*
	 * set the barrier index thread local storage
	 */
	barrier_idx = self->fidx << 32 | self->core_id;	
		
	/*
	 * execute the real thread function
	 */ 
	start_routine = (thread_function) self->func;
	thread_return = start_routine(self->arg);
	
	
	/*
	 * thread finished
	 */
	DPRINTF("%d'th thread (tid %d) finished\n", self->tidx, self->tid);

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
	 * create the new thread
	 */
	tinfo->arg = arg;
	return real_pthread_create(thread, attr, flexpth_thread_wrapper,
				   (void*)tinfo);
	/* return real_pthread_create(thread, attr, start_routine, arg); */
}
