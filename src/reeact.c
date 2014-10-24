/*
 * This file contains the implementation of the constructor and destructor 
 * functions for the REEact library
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "reeact.h"
#include "./utils/reeact_utils.h"
#include "./pthread_hooks/pthread_hooks.h"
#include "./policies/reeact_policy.h"

struct reeact_data *reeact_handle = NULL;

/*
 * REEact initialization procedure for each process
 *
 * Input parameters:
 *     rh: the pointer to the struct reeact_data of this process
 * Return values:
 *     0: success
 *     1: rh is NULL
 */
int reeact_per_proc_init(struct reeact_data *rh)
{
	int pid;

	pid = getpid();
	
	if(rh == NULL){
		fprintf(stderr, "%d %s: reeact_per_proc_init: "
			"REEact data is NULL\n", pid, 
			program_invocation_short_name);
		return 1;
	}

	rh->pid = pid;
	rh->proc_name_short = program_invocation_short_name;
	rh->proc_name_long = program_invocation_name;

	return 0;
}

/*
 * initialization function for REEact
 */
__attribute__ ((__constructor__)) 
void reeact_init(void) 
{
	int ret_val = 0;
	
        DPRINTF("reeact initialization\n");

	// allocate data for reeact
	reeact_handle = 
		(struct reeact_data*)calloc(1, sizeof(struct reeact_data));
	if(reeact_handle == NULL){
		LOGERRX("Unable to allocate memory for REEact data: ");
		return;
	}

	reeact_per_proc_init(reeact_handle);
		
	// pthread hooks initialization
	ret_val = reeact_pthread_hooks_init((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error initializing pthread hooks with error %d\n",
		       ret_val);
	
	// user policy initialization
	ret_val = reeact_policy_init((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error initializing user policy with error %d\n",
		       ret_val);

	return;
}

/*
 * cleanup function for REEact
 */
__attribute__ ((__destructor__)) 
void reeact_cleanup(void) 
{
	int ret_val;

        DPRINTF("reeact cleanup\n");

	ret_val = reeact_policy_cleanup((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error cleaning up pthread hooks with error %d\n",
		       ret_val);

	ret_val = reeact_pthread_hooks_cleanup((void*)reeact_handle);
	if(ret_val != 0)
		LOGERR("Error cleaning up user policy with error %d\n",
		       ret_val);

	if(reeact_handle != NULL)
		free(reeact_handle);

	return;
}
