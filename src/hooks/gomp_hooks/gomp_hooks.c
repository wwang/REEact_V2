/*
 * Initialization and cleanup routines for REEact gomp hooks. These routines are
 * responsible for:
 *     1. opening and closing the original GOMP library
 *     2. providing the original GOMP functions
 *     3. maintaining internal data structures
 *
 * Author: Wei Wang <wwang@virginia.edu?
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>

#include "gomp_hooks.h"
#include "gomp_hooks_originals.h"
#include ".././../utils/reeact_utils.h"

/*
 * real gomp function types
 */
typedef void (*GOMP_barrier_type)(void);
typedef void (*gomp_barrier_init_type)(gomp_barrier_t * bar, unsigned count);
typedef void (*gomp_barrier_general_type)(gomp_barrier_t *bar);
typedef void (*gomp_barrier_state_func_type)(gomp_barrier_t *bar, 
					     gomp_barrier_state_t state);
typedef int (*gomp_barrier_last_thread_type)(gomp_barrier_state_t state);
typedef int (*gomp_team_barrier_waiting_for_tasks_type)(gomp_barrier_t *bar);
typedef void (*gomp_team_barrier_wake_type)(gomp_barrier_t *, int count);
typedef gomp_barrier_state_t 
             (*gomp_barrier_wait_start_type)(gomp_barrier_t *bar);

/*
 * real gomp functions
 */ 
GOMP_barrier_type real_GOMP_barrier;
gomp_barrier_init_type real_gomp_barrier_init;
gomp_barrier_init_type real_gomp_barrier_reinit;
gomp_barrier_general_type real_gomp_barrier_destroy;
gomp_barrier_general_type real_gomp_barrier_wait;
gomp_barrier_general_type real_gomp_barrier_wait_last;
gomp_barrier_state_func_type real_gomp_barrier_wait_end;
gomp_barrier_general_type real_gomp_team_barrier_wait;
gomp_barrier_state_func_type real_gomp_team_barrier_wait_end;
gomp_team_barrier_wake_type real_gomp_team_barrier_wake;
gomp_barrier_wait_start_type real_gomp_barrier_wait_start;
gomp_barrier_last_thread_type real_gomp_barrier_last_thread;
gomp_barrier_general_type real_gomp_team_barrier_set_task_pending;
gomp_barrier_general_type real_gomp_team_barrier_clear_task_pending;
gomp_barrier_general_type real_gomp_team_barrier_set_waiting_for_tasks;
gomp_team_barrier_waiting_for_tasks_type real_gomp_team_barrier_waiting_for_tasks;
gomp_barrier_state_func_type real_gomp_team_barrier_done;


/*
 * initialization function for REEact gomp hooks
 */
int reeact_gomp_hooks_init(void *data)
{
	char *error = NULL;
	int ret_val = 0;
	void *libgomp_handle = NULL;
	DPRINTF("gomp hooks initializing\n");

	/*
	 * locate the original gomp functions
	 */
	dlerror();    // Clear any existing error
       
	/*
	 * we are hooking underlying functions now
	 real_GOMP_barrier = (GOMP_barrier_type)dlsym(RTLD_NEXT, 
	 "GOMP_barrier");
	*/
	real_gomp_barrier_init = 
		(gomp_barrier_init_type)dlsym(RTLD_NEXT, "gomp_barrier_init");
	real_gomp_barrier_reinit = 
		(gomp_barrier_init_type)dlsym(RTLD_NEXT, "gomp_barrier_reinit");
	real_gomp_barrier_destroy = 
		(gomp_barrier_general_type)dlsym(RTLD_NEXT, 
						 "gomp_barrier_destroy");
	real_gomp_barrier_wait = 
		(gomp_barrier_general_type)dlsym(RTLD_NEXT, 
						 "gomp_barrier_wait");
	real_gomp_barrier_wait_last = 
		(gomp_barrier_general_type)dlsym(RTLD_NEXT, 
						 "gomp_barrier_wait_last");
	real_gomp_barrier_wait_end = 
		(gomp_barrier_state_func_type)dlsym(RTLD_NEXT, 
						    "gomp_barrier_wait_end");
	real_gomp_team_barrier_wait = 
		(gomp_barrier_general_type)dlsym(RTLD_NEXT, 
						 "gomp_team_barrier_wait");
	real_gomp_team_barrier_wait_end = (gomp_barrier_state_func_type)
		dlsym(RTLD_NEXT, "gomp_team_barrier_wait_end");
	real_gomp_team_barrier_wake = 
		(gomp_team_barrier_wake_type)dlsym(RTLD_NEXT, 
						   "gomp_team_barrier_wake");
	real_gomp_barrier_wait_start = 
		(gomp_barrier_wait_start_type)dlsym(RTLD_NEXT, 
						    "gomp_barrier_wait_start");
	real_gomp_barrier_last_thread = 
		(gomp_barrier_last_thread_type)dlsym(RTLD_NEXT, 
						 "gomp_barrier_last_thread");
	real_gomp_team_barrier_set_task_pending = (gomp_barrier_general_type)
		dlsym(RTLD_NEXT, "gomp_team_barrier_set_task_pending");
	real_gomp_team_barrier_clear_task_pending = (gomp_barrier_general_type)
		dlsym(RTLD_NEXT, "gomp_team_barrier_clear_task_pending");
	real_gomp_team_barrier_set_waiting_for_tasks = 
		(gomp_barrier_general_type)
		dlsym(RTLD_NEXT, "gomp_team_barrier_set_waiting_for_tasks");
	real_gomp_team_barrier_waiting_for_tasks = 
		(gomp_team_barrier_waiting_for_tasks_type)
		dlsym(RTLD_NEXT, "gomp_team_barrier_waiting_for_tasks");
	real_gomp_team_barrier_done = 
		(gomp_barrier_state_func_type)dlsym(RTLD_NEXT, 
						   "gomp_team_barrier_done");

	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "Error opening original gomp functions with "
			"error :%s\n", error);
		ret_val = REEACT_GOMP_HOOKS_ERR_LOAD_ORIGINAL_FUNCTION;
		goto error;
	}
	
	return 0;

 error:
	return ret_val;
}


/*
 * cleanup function for REEact pthread hooks
 */
int reeact_gomp_hooks_cleanup(void *data)
{
	return 0;
}
