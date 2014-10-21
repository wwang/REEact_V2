/*
 * This file contains the implementation of the constructor and destructor 
 * functions for the REEact library
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */


#include <stdio.h>

#include "./utils/reeact_utils.h"
#include "./pthread_hooks/pthread_hooks.h"

/*
 * initialization function for REEact
 */
__attribute__ ((__constructor__)) 
void reeact_init(void) {
	
        DPRINTF("reeact initialization\n");
	
	reeact_pthread_hooks_init();

	return;
}

/*
 * cleanup function for REEact
 */
__attribute__ ((__destructor__)) 
void reeact_cleanup(void) {
        DPRINTF("reeact cleanup\n");
}
