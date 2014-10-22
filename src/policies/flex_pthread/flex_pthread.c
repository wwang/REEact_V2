/*
 * Initialization and cleanup functions for flex-pthread policy
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdlib.h>
#include <stdio.h>
#include <err.h>

#include "../../reeact.h"
#include "flex_pthread.h"
#include "../../utils/reeact_utils.h"


/*
 * flex-pthread initialization
 */
int flexpth_init(void *data)
{
	struct flexpth_data *fh;
	struct reeact_data *rh = (struct reeact_data*)data;

	DPRINTF("Initializing flex-pthread\n");

	if(rh == NULL){
		warnx("Flexpth init: wrong parameter.\n");
		return 1;
	}

	fh = (struct flexpth_data*)malloc(sizeof(struct flexpth_data));

	if(fh == NULL){
		warn("Flexpth init: unable to create flex-pthead data: ");
		return 2;
	}

	rh->policy_data = (void*)fh;

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
		warnx("Flexpth cleanup: REEact data is NULL.\n");
		return 1;
	}

	if(rh->policy_data){
		warnx("Flexpth cleanup: flex-pthead data is NULL");
		return 2;
	}

	free(rh->policy_data);

	return 0;
}
