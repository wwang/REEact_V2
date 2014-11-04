/*
 * Header file for fast synchronization primitives
 */

#ifndef __FAST_SYNC_H__
#define __FAST_SYNC_H__

typedef struct _fastsync_barrier{
	union{
		struct{
			unsigned int seq; // sequence count
			unsigned int waiting; // number of threads waiting on
			                      //  the barrier at the moment
		};
		unsigned long long reset;
	};
	unsigned int total_count; // total number of threads using this barrier
	struct _fastsync_barrier *parent_d; // a pointer to the parent barrier, 
                                            // used for tree-barrier
	};
}fastsync_barrier;

typedef struct _fastsync_barrier_attr{
	int count;
}fastsync_barrier_attr;

/*
 * initialize a fast sync barrier.
 * Input parameters:
 *     attr: the attribute of the barrier
 *     count: number of threads for this barrier
 * Output parameters:
 *     barrier: the barrier instance
 * Return value:
 *     0: success
 *     1: output parameter barrier is NULL
 */
int fastsync_barrier_init(fastsync_barrier  *barrier,
			  const fastsync_barrier_attr *attr, 
			  unsigned count);

/*
 * wait at a fast sync barrier.
 * Input parameters:
 *     barrier: the barrier to wait at
 *     inc_count: instead of adding 1 to the arriving count, inc_count is added
 * Return value:
 *     0: success
 *     PTHREAD_BARRIER_SERIAL_THREAD: success, this is also the thread that 
 *                                    resets the barrier
 *     -1: unreachable code executed; major error
 */
int fastsync_barrier_wait(fastsync_barrier *barrier);
/*
 * Inter-processor version of the barrier_wait (with spinning instead of block)
 */
int fastsynt_barrier_wait_interproc(fastsync_barrier *barrier, int inc_count);

/*
 * destroy a fast sync barrier.
 * Input parameters:
 *     barrier: the barrier instance
 * Return value:
 *     0: success
 */
int fastsync_barrier_destroy(fastsync_barrier *barrier);


/*
 * Atomic operations
 */

/* Compile Barrier */
#define gcc_barrier() asm volatile("": : :"memory")

/* Atomic add, returning the new value after the addition */
#define atomic_add(P, V) __sync_add_and_fetch((P), (V))

/* Atomic add, returning the value before the addition */
#define atomic_xadd(P, V) __sync_fetch_and_add((P), (V))

/* Force a read of the variable */
#define atomic_read(V) (*(volatile typeof(V) *)&(V))

/* A wrapper for the FUTEX system call; similar to the following function */
/* static long sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3) */
#define sys_futex(addr1, op, val1, timeout, addr2, val3) \
	syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3)

#endif
