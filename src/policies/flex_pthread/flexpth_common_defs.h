/*
 * Common macros and definitions for flex-pthread
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef _FLEXPTH_COMMON_DEFINES_H_
#define _FLEXPTH_COMMON_DEFINES_H_

#define FLEX_PTHREAD_MAX_CORE_CNT 64 // maximum allowed number of cores 
#define FLEXPTH_MAX_BARRIERS   32 // maximum allowed barriers
#define FLEXPTH_MAX_THREAD_FUNCS 32 // maximum allowed thread functions

/* Atomic add, returning the new value after the addition */
#define atomic_subf(P, V) __sync_add_and_fetch((P), (V))

/* Atomic add, returning the value before the addition */
#define atomic_fsub(P, V) __sync_fetch_and_add((P), (V))

/*
 * The static list that represents the tree structure of the used by tree
 * barrier and tree mutex, which in-turn represents the machine topology. The 
 * value of each element is the index of the parent of this element (SLIST_NULL 
 * means no parent).
 */
#define FLEXPTH_BAR_SLIST_NULL -1
struct _flexpth_bar_slist{
	int *elements;
	int len;
};

#endif
