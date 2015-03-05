/*
 * This module is responsible for GNU OpenMP load balancing. GNU OpenMP does not
 * evenly divide the work among its threads. This complicates the load balancing
 * problem when running with many threads. This module is designed to handle the
 * non-even distribution problem.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdio.h>
#include <stdlib.h>

#include "../../reeact.h"
#include "../../utils/reeact_utils.h"

#include "flex_pthread.h"
#include "flexpth_env_var.h"
#include "flexpth_omp_load_balance.h"


int flexpth_omp_load_balance_init(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh;
	struct flexpth_core_list *cl;
	struct flexpth_core_list *omp_cl;
	/* the number of cores that have high loads and low loads; high load 
	 * cores have one more thread than low load cores */
	int h_core_count, l_core_count; 
	int thr_per_core; // how many threads per core for low load cores
	int i,j,n,t;
	int h_cores[4] = {10,14,18,22};

	/*
	 * sanity checks
	 */
	if(rh == NULL){
		LOGERR("REEact data is NULL\n");
		return 1;
	}
	
	fh = (struct flexpth_data*)rh->policy_data;

	if(fh == NULL){
		LOGERR("flex_pthread policy data is NULL\n");
		return 1;
	}

	cl = (struct flexpth_core_list*) fh->core_list;

	if(cl == NULL || fh->omp_thr_cnt == 0){
		LOGERR("core list is NULL (%p) or thread count is zero (%d).\n",
		       cl, fh->omp_thr_cnt);
		return 1;
	}

	/*
	 * do the load balancing
	 */
	/* allocate space for OpenMP core assignment */
	omp_cl = malloc(sizeof(struct flexpth_core_list));
	if(omp_cl == NULL){
		LOGERRX("Error allocating space for OpenMP core list: ");
		return 2;
	}
	omp_cl->cores = malloc(sizeof(int) * fh->omp_thr_cnt);
	if(omp_cl->cores == NULL){
		LOGERRX("Error allocating space for OpenMP core list array: ");
		return 2;
	}
	omp_cl->core_cnt = fh->omp_thr_cnt;
	fh->omp_core_list = (void*)omp_cl;

	/* how many cores have one more thread than others? */
	h_core_count = fh->omp_thr_cnt % cl->core_cnt;
	l_core_count = cl->core_cnt - h_core_count;
	thr_per_core = fh->omp_thr_cnt / cl->core_cnt;
	DPRINTF("high load core count: %d, low load core count: %d, OMP thread "
		"count: %d, threads per core: %d\n", h_core_count, l_core_count,
		fh->omp_thr_cnt, thr_per_core);

	/* assign threads to low load cores in a round-robin fashion*/
	/* n = thr_per_core * l_core_count; */
	/* for(i = 0; i < n; i++){ */
	/* 	t = i % l_core_count; */
	/* 	omp_cl->cores[i] = cl->cores[t]; */
	/* } */
	/* n = 0; */
	/* for(j = 0; j < cl->core_cnt; j++){ */
	/* 	t = j; */
	/* 	for(i = 0; i < thr_per_core; i++) */
	/* 		omp_cl->cores[n++] = cl->cores[t]; */
	/* } */
		
	/* assign threads to high load cores by grouping them together;
	   note that the load balancing here is per-core base; there are also
           load balancing needs on the basis of nodes */
	/* for(j = 0; j < h_core_count; j++){ */
	/* 	t = l_core_count + j; */
	/* 	for(i = 0; i <= thr_per_core; i++) */
	/* 		omp_cl->cores[n++] = cl->cores[t]; */
	/* } */
	/* t = 0; */
	/* for(; n < fh->omp_thr_cnt; n++){ */
	/* 	omp_cl->cores[n] = cl->cores[t++]; */
	/* } */


	/* assign threads to cores in round-robin fashion until the last 
	   h_core_count threads */
	n = fh->omp_thr_cnt - h_core_count;
	for(i = 0; i < n; i++){
		t = i % cl->core_cnt;
		omp_cl->cores[i] = cl->cores[t];
	}
	/* for the last h_core_count threads, assign them to last cores */
	t = cl->core_cnt - h_core_count;
	//t = 0;
	for(; i < fh->omp_thr_cnt; i++){
		omp_cl->cores[i] = cl->cores[t++];
		//omp_cl->cores[i] = h_cores[t++];
	}

	
#ifdef _REEACT_DEBUG_
	/*
	 * log the core allocation
	 */
	DPRINTF("OMP core allocation: ");
	for(i = 0; i < fh->omp_thr_cnt; i++)
		fprintf(stderr, "%d,", omp_cl->cores[i]);
	fprintf(stderr, "\n");
#endif

	return 0;
}


int flexpth_omp_load_balance_cleanup(void *data)
{
	struct reeact_data *rh = (struct reeact_data*)data;
	struct flexpth_data *fh = (struct flexpth_data*)rh->policy_data;
	
	if(rh == NULL || fh == NULL){
		LOGERR("REEact data (%p) or flex-pthread data (%p) is NULL\n",
		       rh, fh);
		return 1;
	}

	if(fh->omp_core_list != NULL){
		struct flexpth_core_list *omp_cl =
			(struct flexpth_core_list*)fh->omp_core_list;
		if(omp_cl->cores != NULL)
			free(omp_cl->cores);
		free(omp_cl);
	}
	
	return 0;
}
