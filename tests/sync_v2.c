/*
 * Simple program to test the overhead of synchronizing barriers.               
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <err.h>
#include <dlfcn.h>
#include <sys/time.h>

#include <common_toolx.h>

#include "sync_worker_func.h"

#define MAX_CORES 256 // the maximum number of cores this program can use
#define MAX_THREADS 16384 // the maximum number of threads

typedef struct _cmd_params{ // data structure for command line parameters
	int thr_cnt; // worker thread count
	int * cores; // the cores to use
	int core_cnt; // the number of cores to use
	unsigned long long total_iters; // the total number of iterations to do
	unsigned long long bar_iters; // iterations to do per barrier
	char * lib_name; // the name of library
	char * func_name; // the name of working function
	void * lib; // the handle to the library
	worker_func func; // pointer to the worker function
	int debug; // enable debug output
        int verbose; // enable verbose output
	int all_start; // the predicate that all threads can start
	int sync_type; // the type of synchronization
}cmd_params;

typedef struct _thr_params{ // data structure for thread function parameters
	int tidx; // the index of the worker thread
	cmd_params * cmd_params; // command line parameters
	unsigned long long ret_val; // return value from the thread function
	pthread_barrier_t * sync_point; // the synchronization point (barrier) 
	                                // to wait at
	unsigned long long total_iters; // total iterations for this thread
	unsigned long long extra_trial; // there is an extra trial for this 
                                        // thread
	pthread_cond_t *all_start; // "all-start" condition for all threads
	pthread_mutex_t *cond_mtx; // the mutex for the "all-start" condition
	pthread_mutex_t *mutex; // the mutex for synchronization test
}thr_params;

/*
 * a global counter for critical section test
 */
unsigned long long critical_counter = 0;

/*
 * initialized the values of the command line parameters
 */
int init_parameters(cmd_params * p)
{
	p->thr_cnt = 0;
	p->cores = (int*)malloc(sizeof(int) * MAX_CORES);
	p->core_cnt = 0;
	p->total_iters = 0;
	p->bar_iters = 0;
	p->lib_name = NULL;
	p->func_name = NULL;
	p->lib = NULL;
	p->func = NULL;
	p->debug = 0;
	p->verbose = 0;
	p->all_start = 0;
	p->sync_type = 0;

	return 0;
}

/*
 * log the user-input parameters
 */

int print_parameters(cmd_params *p)
{
	int i;
	
	printf("Input parameters:\n");
	printf("\t thread count: %d\n", p->thr_cnt);
	printf("\t core count: %d\n", p->core_cnt);

	printf("\t core list: ");
	for(i = 0; i < p->core_cnt; i++)
		printf("%d,", p->cores[i]);

	printf("\n");

	printf("\t total iterations: %llu\n", p->total_iters);
	printf("\t per-barrier iterations: %llu\n", p->bar_iters);
	printf("\t library name: %s\n", p->lib_name);
	printf("\t function name: %s\n", p->func_name);
	printf("\t synchronization type: %d\n", p->sync_type);

	return 0;
}


/*
 * print the program usage (the command line parameter usage)
 */
int print_usage()
{
	char * usage = 
		"Usage: sync [options]\n"
		"\n"
		"Options:\n"
		"  -h, --help"
		"\t show this help message and exit\n"
		"  -t THREAD_COUNT, --threads=THREAD_COUNT\n"
		"\t how many threads to create\n"
		"  -c CORES, --cores=CORES\n"
		"\t list of the cores on which the threads will run, CORES "
		"should be comma separated list\n"
		"  -m TOTAL_ITERS, --iters=TOTAL_ITERS\n"
		"\t the total number of iterations to run; these iterations "
		"will be evenly distributed among the worker threads\n"
		"  -n BAR_ITERS, --loops=BAR_ITERS\n"
		"\t the number of iterations to run before waiting at the "
		"barrier\n"
		" -l LIBRARY --lib=LIBRARY\n"
		"\t the name of the library with the working function\n"
		" -f FUNCTION --func=FUNCTION\n"
		"\t the name of the working function\n"
		" -s SYNC_TYPE --sync=SYNC_TYPE\n"
		"\t the type of synchronization: 0: barrier, 1: mutex, 2: "
		"conditional variable; default 0\n"
		"  -d, --debug\n"
		"\t enable debug output\n"
		"  -v, --verbose\n"
		"\t enable verbose output.\n";

		
	fprintf(stderr, "%s\n", usage);
	
	return 0;
}

/*
 * Parse the input parameters from command line
 */
int parse_parameters(int argc, char * argv[], cmd_params *p)
{
	struct option long_params[] = {
		{"threads", required_argument, 0, 1001},
		{"cores", required_argument, 0, 1002},
		{"iters", required_argument, 0, 1003},
		{"loops", required_argument, 0, 1004},
		{"lib", required_argument, 0, 1005},
		{"func", required_argument, 0, 1006},
		{"debug", no_argument, 0, 1007},
		{"verbose", no_argument, 0, 1008},
		{"help", no_argument, 0, 1009},
		{"sync", required_argument, 0, 1010},
		{0, 0, 0, 0},
	};

	int long_index = 0;
	int opt = 0;
	int ret_val = 0;
	
	while((opt = getopt_long(argc, argv, "t:c:m:n:l:f:s:dvh", long_params,
				 &long_index)) != -1){
		switch (opt) {
		case 't':
		case 1001:
			p->thr_cnt = atoi(optarg);
			break;
		case 'c':
		case 1002:
			ret_val = parse_value_list(optarg, (void**)&(p->cores),
						   &(p->core_cnt), 0);
			if(ret_val != 0){
				fprintf(stderr, "Unable to parse core list: "
					"error %d\n", ret_val);
				goto error;
			}
			break;
		case 'm':
		case 1003:
			p->total_iters = strtoull(optarg, NULL, 0);
			break;
		case 'n':
		case 1004:
			p->bar_iters = strtoull(optarg, NULL, 0);
			break;
		case 'l':
		case 1005:
			p->lib_name = strdup(optarg);
			break;
		case 'f':
		case 1006:
			p->func_name = strdup(optarg);
			break;
		case 'd':
		case 1007:
			p->debug = 1;
			break;
		case 'v':
		case 1008:
			p->verbose = 1;
			break;
		case 'h':
		case 1009:
			print_usage();
			exit(0);
		case 's':
		case 1010:
			p->sync_type = atoi(optarg);
			break;
		default:
			goto error;
			break;
		}
	}
	
	// check the command line parameters
	if (p->total_iters == 0){
		fprintf(stderr, "Please specify the total number of "
			"iterations.\n");
		goto error;
	}

	if (p->bar_iters == 0){
		fprintf(stderr, "Please specify the number of iterations"
			"before waiting for each barrier.\n");
		goto error;
	}

	if (p->thr_cnt == 0){
		fprintf(stderr, "Please specify the number of threads.\n");
		goto error;
	}

	if (p->thr_cnt > MAX_THREADS){
		fprintf(stderr, "The number of threads should be smaller"
			"than %d.\n", MAX_THREADS);
		goto error;
	}
	   

	if (p->core_cnt == 0){
		fprintf(stderr, "Please specify the cores to run.\n");
		goto error;
	}

	if (p->lib_name == NULL){
		fprintf(stderr, "Please specify the library which contains "
			"the working function\n");
		goto error;
	}
	
	if (p->func_name == NULL){
		fprintf(stderr, "Please specify the name of the working "
			"function\n");
		goto error;
	}

	// log out the parameters
	if (p->verbose)
		print_parameters(p);
	
	// all good
	return 0;

	
 error: // generic error handling
	print_usage();
	exit(-1);
}

double get_elapsed_time(struct timeval *start, struct timeval *end)
{
	double t, ts, te;

	ts = start->tv_sec * 1000000 + start->tv_usec;
	te = end->tv_sec * 1000000 + end->tv_usec;
	
	t = (te - ts)/1000000;

	return t;
}

// the thread function of the worker thread
void * thread_func(void * thr_args)
{
	int ret_val;
	unsigned long long trials_done = 0;
	unsigned long long trials_to_do = 0;
	thr_params * args = (thr_params*)thr_args;
	cmd_params * p = args->cmd_params;
	unsigned long long int sync_called = 0;
	struct timeval start, end;
	
	args->ret_val = 0;

	// log out thread parameters
	if(p->verbose){
		printf("Thread %d -- iterations: %llu\n", args->tidx, 
		       args->total_iters);
		printf("Thread %d -- extra trial: %llu\n", args->tidx,
		       args->extra_trial);
	}

	// wait for other threads to be ready
	ret_val = pthread_barrier_wait(args->sync_point);

	// wait for the "all-start" signal from the parent
	//ret_val = pthread_mutex_lock(args->cond_mtx);
	while(!p->all_start)
		asm volatile("pause\n": : :"memory");
	//	ret_val = pthread_cond_wait(args->all_start, args->cond_mtx);
	//ret_val = pthread_mutex_unlock(args->cond_mtx);
	
	printf("Worker thread %d start\n", args->tidx);
	// get start time 
	gettimeofday(&start, NULL);
	while(trials_done < args->total_iters){
		// figure out how many iterations to run
		if((args->total_iters - trials_done) >= p->bar_iters)
			trials_to_do = p->bar_iters;
		else
			trials_to_do = args->total_iters - trials_done;
		// do the iteration
		args->ret_val += p->func((void*)&(trials_to_do));
		trials_done += p->bar_iters;
		// do the synchronization
		switch(p->sync_type){
		case 1:
			ret_val = pthread_mutex_lock(args->mutex);
			sync_called++;
			critical_counter++;
			ret_val = pthread_mutex_unlock(args->mutex);
			ret_val = pthread_barrier_wait(args->sync_point);
			break;
		case 2:
			sync_called++;
			ret_val = pthread_barrier_wait(args->sync_point);
			if(args->tidx == 0)
				p->all_start = 0;
			ret_val = pthread_barrier_wait(args->sync_point);
			if(args->tidx == 0){
				ret_val = pthread_mutex_lock(args->cond_mtx);
				p->all_start = 1;
				//critical_counter++;
				ret_val = pthread_mutex_unlock(args->cond_mtx);
				pthread_cond_broadcast(args->all_start);
			}
			else{
				ret_val = pthread_mutex_lock(args->cond_mtx);
				//critical_counter++;
				while(!p->all_start)
					pthread_cond_wait(args->all_start, 
							  args->cond_mtx);
				ret_val = pthread_mutex_unlock(args->cond_mtx);
			}
		case 0:
		default:
			sync_called++;
			ret_val = pthread_barrier_wait(args->sync_point);
			if(ret_val != 0 && 
			   ret_val != PTHREAD_BARRIER_SERIAL_THREAD) 
				warn("Error waiting for barrier");
		}
	}

	// do the extra trial
	if(args->extra_trial)
		args->ret_val += p->func((void*)(&args->extra_trial));

	// get finish time
	gettimeofday(&end, NULL);

	printf("Worker thread %d finished with result %llu (%llu sync "
	       "called) in %f seconds.\n", args->tidx, args->ret_val, 
	       sync_called, get_elapsed_time(&start, &end));
	
	return NULL;
}

/*
 * Open the worker function specified the shared library specified by the user
 */
int open_worker_func(cmd_params * p)
{
	char *error = NULL;

	// open the library
	p->lib = dlopen(p->lib_name, RTLD_LAZY);
	if(p->lib == NULL) {
		fprintf(stderr, "Error opening library %s with error %s\n",
			p->lib_name, dlerror());
		exit(4);
	}

	dlerror();    // Clear any existing error

	// find the worker function
	p->func = dlsym(p->lib, p->func_name);

	if ((error = dlerror()) != NULL)  {
		fprintf(stderr, "Error opening worker function %s with error "
			"%s\n", p->func_name, error);
		exit(5);
	}

	return 0;
}

int main(int argc, char * argv[])
{
	cmd_params params;
	int t;
	//int c;
	pthread_t threads[MAX_THREADS];
	thr_params thr_args[MAX_THREADS];
	int ret_val;
	//cpu_set_t core_id;
	pthread_barrier_t sync_point;
	unsigned long long thr_trials, extra_trials;
	struct timeval start, end;
	pthread_cond_t all_start  = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t cond_mtx = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
	
	// read in command line parameters
	init_parameters(&params);

	parse_parameters(argc, argv, &params);

	// open the worker function
	open_worker_func(&params);

	// initialized the barrier
	ret_val = pthread_barrier_init(&sync_point, NULL, params.thr_cnt);
	if(ret_val != 0)
		err(3, "Error initializing barrier");

	// create worker threads
	thr_trials = params.total_iters / params.thr_cnt; // trials per thread
	extra_trials = params.total_iters % params.thr_cnt; // some extra trials
	                                                    // cannot distribute
                                                            // evenly
	for(t = 0; t < params.thr_cnt; t++){
		// setup thread parameters to pass
		thr_args[t].tidx = t;
		thr_args[t].cmd_params = &params;
		thr_args[t].sync_point = &sync_point;
		thr_args[t].cond_mtx = &cond_mtx;
		thr_args[t].all_start = &all_start;
		thr_args[t].total_iters = thr_trials;
		thr_args[t].mutex = &mtx;
		if(extra_trials > 0){
			thr_args[t].extra_trial = 1;
			extra_trials--;
		}
		else
			thr_args[t].extra_trial = 0;
		// create the thread
		ret_val = pthread_create(&(threads[t]), NULL, thread_func,
					 &(thr_args[t]));
		if(ret_val != 0)
			err(1, "Error creating new thread:");

		/* // pin the thread to a core */
		/* c = t % params.core_cnt; */
		/* CPU_ZERO(&core_id); */
		/* CPU_SET(params.cores[c], &core_id); */
		/* ret_val = pthread_setaffinity_np(threads[t], sizeof(cpu_set_t),  */
		/* 				 &core_id); */
		/* if(ret_val != 0) */
		/* 	err(2, "Error pinning new thread:"); */
	}

	// let all threads start
	//ret_val = pthread_mutex_lock(&cond_mtx);
	params.all_start = 1;
	//ret_val = pthread_mutex_unlock(&cond_mtx);
	//ret_val = pthread_cond_broadcast(&all_start);

	// get the start time
	gettimeofday(&start, NULL);

	// wait for worker threads to quit
	for(t = 0; t < params.thr_cnt; t++){
		ret_val = pthread_join(threads[t], NULL);
	}

	// get the finish time
	gettimeofday(&end, NULL);

	// output results
	printf("All threads finished in %f seconds\n", 
	       get_elapsed_time(&start, &end));
	printf("Critical counter value is %llu\n", critical_counter);

	// clean up
	pthread_barrier_destroy(&sync_point);
	dlclose(params.lib);
	
	return 0;
}
