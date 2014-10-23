/*
 * Logging facilities for REEact.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>


/*
 * Normal debug logging to stderr output. Return value is the same as vfprintf.
 *
 */
int reeact_dprintf(const char* fmt, ...)
{
	int ret_val;
	va_list argptr;

	va_start(argptr, fmt);
	ret_val = vfprintf(stderr, fmt, argptr);
	va_end(argptr);
	
	return ret_val;
}

/*
 * Error logging without append system error message. Return value same as 
 * vfprintf.
 */
int reeact_log_err(const char *fmt, ...)
{
	int ret_val;
	va_list argptr;

	va_start(argptr, fmt);
	ret_val = vfprintf(stderr, fmt, argptr);
	va_end(argptr);
	
	return ret_val;
}

/*
 * Error logging with system error message appended. Return values same as
 * fprintf.
 */
int reeact_log_errx(const char *fmt, ...)
{
	int ret_val;
	va_list argptr;
	int err = errno;
	
	// print user message
	va_start(argptr, fmt);
	ret_val = vfprintf(stderr, fmt, argptr);
	va_end(argptr);
	// append system error message
	ret_val = fprintf(stderr, "%s(%d)\n", sys_errlist[err], err);
	
	return ret_val;
} 
