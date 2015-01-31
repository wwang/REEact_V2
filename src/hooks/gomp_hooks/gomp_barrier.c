/*
 * Implementation of gomp barrier functions. Most internal functions of barriers
 * (start with "gomp") are compiled as inline functions in the libgomp library. 
 * To hook those functions, libgomp has to recompiled.
 * 
 * IMPORTANT:
 *   Sadly, I learned that these hooks are not working. LD_PRELOAD only works if
 *   the functions are called cross libraries. The following gomp barrier 
 *   functions are all called within the libgomp library. Therefore, they cannot
 *   be intercepted by LD_PRELOAD. A reasonable thing that I misunderstood, somehow.
 *   Now it leaves me with two options: recompile libgomp with -Wl wrapper, or 
 *   change libgomp to call reeact functions.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include "gomp_hooks.h"

#include "../../utils/reeact_utils.h"
#include "../../policies/reeact_policy.h"

/* void GOMP_barrier() */
/* { */
/* 	DPRINTF("GOMP_barrier called\n"); */

/* 	return reeact_policy_GOMP_barrier(); */
/* } */

void gomp_barrier_init(gomp_barrier_t *bar, unsigned count)
{
	DPRINTF("%s called with bar %p, count %u\n", __FUNCTION__, bar, count);
	return reeact_gomp_barrier_init(bar, count);
}

void gomp_barrier_reinit(gomp_barrier_t *bar, unsigned count)
{
	DPRINTF("%s called with bar %p, count %u\n", __FUNCTION__, bar, count);
	return reeact_gomp_barrier_reinit(bar, count);
}

void gomp_barrier_destroy(gomp_barrier_t *bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_barrier_destroy(bar);
}

void gomp_barrier_wait(gomp_barrier_t *bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_barrier_wait(bar);
}

void gomp_barrier_wait_last(gomp_barrier_t *bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_barrier_wait_last(bar);
}

void gomp_barrier_wait_end(gomp_barrier_t *bar, gomp_barrier_state_t state)
{
	DPRINTF("%s called with bar %p and state %d\n", __FUNCTION__, bar, state);
	return reeact_gomp_barrier_wait_end(bar, state);
}

void gomp_team_barrier_wait(gomp_barrier_t *bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_team_barrier_wait(bar);
}

void gomp_team_barrier_wait_end(gomp_barrier_t *bar, 
				 gomp_barrier_state_t state)
{
	DPRINTF("%s called with bar %p and state %d\n", __FUNCTION__, bar, state);
	return reeact_gomp_team_barrier_wait_end(bar, state);
}

void gomp_team_barrier_wake(gomp_barrier_t *bar, int count)
{
	DPRINTF("%s called with bar %p and count %d\n", __FUNCTION__, bar, count);
	return reeact_gomp_team_barrier_wake(bar, count);
}

gomp_barrier_state_t gomp_barrier_wait_start(gomp_barrier_t *bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_barrier_wait_start(bar);
}

int gomp_barrier_last_thread(gomp_barrier_state_t state)
{
	DPRINTF("%s called with state %d\n", __FUNCTION__, state);
	return reeact_gomp_barrier_last_thread(state);
}

void gomp_team_barrier_set_task_pending(gomp_barrier_t * bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_team_barrier_set_task_pending(bar);
}

void gomp_team_barrier_clear_task_pending(gomp_barrier_t *bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_team_barrier_clear_task_pending(bar);
}

void gomp_team_barrier_set_waiting_for_tasks(gomp_barrier_t *bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_team_barrier_set_waiting_for_tasks(bar);
}

int gomp_team_barrier_waiting_for_tasks(gomp_barrier_t *bar)
{
	DPRINTF("%s called with bar %p\n", __FUNCTION__, bar);
	return reeact_gomp_team_barrier_waiting_for_tasks(bar);
}

void gomp_team_barrier_done(gomp_barrier_t *bar, gomp_barrier_state_t state)
{
	DPRINTF("%s called with bar %p and state %d\n", __FUNCTION__, bar, state);
	return reeact_gomp_team_barrier_done(bar, state);
}

