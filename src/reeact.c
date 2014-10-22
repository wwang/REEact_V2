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
void reeact_init(void) {
	
        DPRINTF("reeact initialization\n");

	// allocate data for reeact
	reeact_handle = (struct reeact_data*)malloc(sizeof(struct reeact_data));

	// pthread hooks initialization
	reeact_pthread_hooks_init();
	
	// user policy initialization
	reeact_policy_init((void*)reeact_handle);

	return;
}

/*
 * cleanup function for REEact
 */
__attribute__ ((__destructor__)) 
void reeact_cleanup(void) {
        DPRINTF("reeact cleanup\n");

	if(reeact_handle != NULL)
		free(reeact_handle);

	return;
}
