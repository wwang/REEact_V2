/*
 * Header file for the module that handles the GNU OpenMP load balancing.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __FLEXPTH_OMP_LOAD_BALANCE_H__
#define __FLEXPTH_OMP_LOAD_BALANCE_H__


/*
 * Initialization and cleanup functions for GNU OpenMP load balancing modules.
 * The initialization function should be called after the environment variables
 * are processed.
 *
 * Input Parameters:
 *      data: a pointer to REEact data (struct reeact_data *)
 * Return value:
 *      0: success
 *      1: wrong parameters
 *      2: memory allocation error
 */
int flexpth_omp_load_balance_init(void *data);
int flexpth_omp_load_balance_cleanup(void *data);


#endif
