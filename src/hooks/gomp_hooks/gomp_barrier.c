/*
 * Implementation of gomp barrier functions. Most internal functions of barriers
 * (start with "gomp") are compiled as inline functions in the libgomp library. 
 * To hook those functions, libgomp has to recompiled.
 * 
 * IMPORTANT:
 *   Sadly, I learned that hooking gomp_barrier is not working. LD_PRELOAD only 
 *   works if the functions are called cross libraries. The following gomp 
 *   barrier functions are all called within the libgomp library. Therefore, 
 *   they cannot be intercepted by LD_PRELOAD. A reasonable thing that I 
 *   misunderstood, somehow.
 *
 *   Now it leaves me with two options: recompile libgomp with -Wl wrapper, or 
 *   change libgomp to call reeact functions. I choose to change libgomp's bar.c
 *   file, because I do not feel liking touching the convoluted gcc compilation
 *   flags. More specifically, I will let the gomp barrier functions in bar.c to
 *   their corresponding reeact functions. If reeact functions returns 0, then
 *   the gomp barrier functions should stop and return. Otherwise, the gomp 
 *   barriers should proceed to their original implementations.
 *   
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
