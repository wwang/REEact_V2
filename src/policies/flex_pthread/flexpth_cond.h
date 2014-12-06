/*
 * Header file for the flex-pthread distributed conditional variable.
 *
 * Author: Wei Wang <wwang@virginia.ed>
 */

#ifndef __FLEX_PTHREAD_COND_VAR_H__
#define __FLEX_PTHREAD_COND_VAR_H__

#include "../../fastsync/fastsync.h"

#include "flexpth_common_defs.h"

/*
 * data structure for the flex-pthread conditional variable attributes
 */
struct flexpth_cond_attr{
	// TODO: add more attributes
	int dummy;
};

/*
 * data structure that holds the information of one flex-pthread conditional
 * variable. This data structure will be used to replace the content of 
 * pthread_cond_t, which is about 48 bytes. 
 */
struct flexpth_cond{
	/*
	 * The magic number that indicates this mutex if a flex-pthread 
	 * conditional variable. Because users can initialize a pthread 
	 * conditional variable without calling the pthread_cond_init function, 
	 * I have to initialize a cond var on the first call of cond_wait or
	 * cond_signal. For the following calls, the conditional variable can
	 * be safely used. To determine whether the conditional variable has
	 * bee properly initialized, I introduced this magic number.
	 */
	int magic_number;
	int len; // the length of the per-core conditional variable list
	int first_core_idx; // the index of the first core-level conditional
	                    // variable in the list
	void *conds; // the list of distributed per-core conditional variables

};


/*
 * Initialization function for flex_pthread conditional variable component.
 *
 * Input Parameters:
 *      data: a pointer to REEact data (struct reeact_data *)
 * Return value:
 *      0: success
 *      1: wrong parameters
 *      2: memory allocation error
 */
int flexpth_cond_internal_init(void *data);
int flexpth_cond_internal_cleanup(void *data);


/*
 * Initialize a new flex_pthread distributed conditional variable.
 *
 * Input parameters:
 *     data: a pointer to REEact data
 *     attr: conditional variable attributes
 * Output parameters:
 *     cond: the distributed conditional variable to initialized
 * Return values:
 *     0: success
 *     1: wrong parameter 
 *     2: topology information not ready
 *     3: Unable to allocate memory
 *     
 */
int flexpth_distribute_cond_init(void *data, struct flexpth_cond *cond,
				  struct flexpth_cond_attr *attr);


#endif
