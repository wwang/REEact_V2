/*
 * Implementation of gomp barrier functions. Most internal functions of barriers
 * (start with "gomp") are compiled as inline functions in the libgomp library. 
 * To hook those functions, libgomp has to recompiled.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include "gomp_hooks.h"

#include "../../utils/reeact_utils.h"
#include "../../policies/reeact_policy.h"

/* void gomp_team_barrier_wait(gomp_barrier_t *gbar) */
/* { */
/* 	DPRINTF("gomp_team_barrier_wait called: %p\n", gbar); */

/* 	return reeact_policy_gomp_barrier_wait((void*)gbar); */
/* } */

void GOMP_barrier()
{
       	/* DPRINTF("GOMP_barrier called\n"); */

	return reeact_policy_GOMP_barrier();
}
