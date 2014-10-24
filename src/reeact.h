/*
 * Header file for REEact. The internal data structure used by REEact is defined
 * here.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 *
 */

#ifndef __REEACT_FRAMEWORK_H__
#define __REEACT_FRAMEWORK_H__

struct reeact_data{
	int pid;
	char *proc_name_short; // process name long
	char *proc_name_long; // process base name (without path)
	void *policy_data; // pointing to the user-specified policy data
};

#endif
