/*
 * Utility functions for REEact.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __REEACT_UTILITY_H__
#define __REEACT_UTILITY_H__

/* 
 * Compilation time controlled debug output
 */
#ifdef _REEACT_DEBUG_
#define DPRINTF(fmt, ...)			\
	do { fprintf(stderr, fmt ,			\
		     ## __VA_ARGS__);} while (0);
#else
#define DPRINTF(fmt, ...)			\
	do {} while(0);
#endif

/*
 * Execution time controlled debug output
 */
#define dprintf(debug,fmt, ...)					\
	do { if(debug) {fprintf(stderr, fmt ,	\
			       ## __VA_ARGS__);}} while (0);


#endif
