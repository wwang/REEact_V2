/*
 * Header file that defines the wrappers of the original gomp functions. Include
 * this file if the original functions are required.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __REEACT_GOMP_HOOKS_ORIGINALS_H__
#define __REEACT_GOMP_HOOKS_ORIGINALS_H__

#include "gomp_hooks.h" 

//extern void (*real_GOMP_barrier)(void);
extern void (*real_gomp_barrier_init)(gomp_barrier_t *, unsigned);
extern void (*real_gomp_barrier_reinit)(gomp_barrier_t *, unsigned);
extern void (*real_gomp_barrier_destroy)(gomp_barrier_t *);
extern void (*real_gomp_barrier_wait)(gomp_barrier_t *);
extern void (*real_gomp_barrier_wait_last)(gomp_barrier_t *);
extern void (*real_gomp_barrier_wait_end)(gomp_barrier_t *, 
					  gomp_barrier_state_t);
extern void (*real_gomp_team_barrier_wait)(gomp_barrier_t *);
extern void (*real_gomp_team_barrier_wait_end)(gomp_barrier_t *,
					       gomp_barrier_state_t);
extern void (*real_gomp_team_barrier_wake)(gomp_barrier_t *, int);
extern gomp_barrier_state_t 
            (*real_gomp_barrier_wait_start)(gomp_barrier_t *);
extern int (*real_gomp_barrier_last_thread)(gomp_barrier_state_t);
extern void (*real_gomp_team_barrier_set_task_pending)(gomp_barrier_t *);
extern void (*real_gomp_team_barrier_clear_task_pending)(gomp_barrier_t *);
extern void (*real_gomp_team_barrier_set_waiting_for_tasks)(gomp_barrier_t *);
extern int (*real_gomp_team_barrier_waiting_for_tasks)(gomp_barrier_t *);
extern void (*real_gomp_team_barrier_done)(gomp_barrier_t *, 
					   gomp_barrier_state_t);



#endif
