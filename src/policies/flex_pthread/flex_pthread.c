/*
 * Initialization and cleanup functions for flex-pthread policy
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdlib.h>
#include <stdio.h>
#include <err.h>

#include "../../reeact.h"
#include "../../utils/reeact_utils.h"

#include "flex_pthread.h"
#include "flexpth_barrier.h"
#include "flexpth_thread_keeper.h"
#include "flexpth_env_var.h"

/*
 * flex-pthread initialization
 */
int flexpth_init(void *data)
{
	struct flexpth_data *fh;
	struct reeact_data *rh = (struct reeact_data*)data;

	DPRINTF("Initializing flex-pthread\n");

	if(rh == NULL){
		LOGERR("Flexpth init: wrong parameter.\n");
		return 1;
	}

	fh = (struct flexpth_data*)malloc(sizeof(struct flexpth_data));

	if(fh == NULL){
		LOGERRX("Flexpth init: unable to create flex-pthead data: ");
		return 2;
	}

	rh->policy_data = (void*)fh;

	/*
	 * per component initialization
	 */
	flexpth_parse_env_vars(data);
	flexpth_barrier_internal_init(data);
	flexpth_thread_keeper_init(data);

	return 0;
}

/*
 * flex-pthread cleanup
 */ 
int flexpth_cleanup(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;

	DPRINTF("Cleaning up flex-pthread\n");

	if(rh == NULL){
		LOGERR("Flexpth cleanup: REEact data is NULL.\n");
		return 1;
	}

	if(rh->policy_data == NULL){
		LOGERR("Flexpth cleanup: flex-pthead data is NULL");
		return 2;
	}

	/*
	 * per component cleanup
	 */
	flexpth_barrier_internal_cleanup(data);
	flexpth_thread_keeper_cleanup(data);
	flexpth_env_vars_cleanup(data);

	free(rh->policy_data);

	return 0;
}