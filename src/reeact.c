/*
 * This file contains the implementation of the constructor and destructor 
 * functions for the REEact library
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */


#include <stdio.h>
#include <stdlib.h>

#include "reeact.h"
#include "./utils/reeact_utils.h"
#include "./pthread_hooks/pthread_hooks.h"
#include "./policies/reeact_policy.h"

struct reeact_data *reeact_handle = NULL;

/*
 * initialization function for REEact
 */
__attribute__ ((__constructor__)) 
void reeact_init(void) 
{
	int ret_val = 0;
	
        DPRINTF("reeact initialization\n");

	// allocate data for reeact
	reeact_handle = (struct reeact_data*)calloc(1,
						    sizeof(struct reeact_data));
	if(reeact_handle == NULL){
		LOGERRX("Unable to allocate memory for REEact data: ");
		return;
	}
		
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
