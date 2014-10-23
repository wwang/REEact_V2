/*
 * Shared library that contains various worker functions for the sync benchmark/
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdio.h>

#include "sync_worker_func.h"

unsigned long long add_worker(void * args)
{
	unsigned long long trials = *((unsigned long long*)args);
	unsigned long long i;
	unsigned long long result = 0;

	for(i = 0; i < trials; i++)
		result += 1;
	 
	return result;
}

unsigned long long div_worker(void * args)
{
	unsigned long long trials = *((unsigned long long*)args);
 	unsigned long long i;
	unsigned long long result = 0;

	for(i = 1; i <= trials; i++){
		result /= i;
		result += i;
	}
	 
	return result;
}

unsigned long long mul_worker(void * args)
{
	unsigned long long trials = *((unsigned long long*)args);
	unsigned long long i;
	unsigned long long result = 1;

	for(i = 1; i <= trials; i++)
		result *= i;
	 
	return result;
}
