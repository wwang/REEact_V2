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
#define DPRINTF(fmt, ...)					\
	do { reeact_dprintf("%s: "fmt , __func__,		\
			    ## __VA_ARGS__);} while (0);
#else
#define DPRINTF(fmt, ...)			\
	do {} while(0);
#endif

/*
 * Execution time controlled debug output
 */
#define dprintf(debug,fmt, ...)						\
	do { if(debug) {reeact_dprintf("%s: "fmt , __func__,		\
				       ## __VA_ARGS__);}} while (0);

/*
 * Error logging without system error message
 */ 
#define LOGERR(fmt,...)							\
	do{ reeact_log_err("%s: "fmt, __func__,				\
			   ## __VA_ARGS__);} while (0);

/*
 * Error logging with system error message
 */ 
#define LOGERRX(fmt,...)						\
	do{ reeact_log_errx("%s: "fmt, __func__,			\
			    ## __VA_ARGS__);} while (0);


/*
 * Normal debug logging to stderr output. Return value is the same as vfprintf.
 *
 */
int reeact_dprintf(const char* fmt, ...);

/*
 * Error logging without append system error message. Return value same as 
 * vfprintf.
 */
int reeact_log_err(const char *fmt, ...);

/*
 * Error logging with system error message appended. Return values same as
 * fprintf.
 */
int reeact_log_errx(const char *fmt, ...);


#endif
