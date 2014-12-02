/*
 * Header file for fast synchronization primitives
 */

#ifndef __FAST_SYNC_H__
#define __FAST_SYNC_H__


/*
 * BEGIN: Atomic operations and other common definitions
 */

/* Compile Barrier */
#define gcc_barrier() asm volatile("": : :"memory")

/* Atomic add, returning the new value after the addition */
#define atomic_addf(P, V) __sync_add_and_fetch((P), (V))

/* Atomic add, returning the value before the addition */
#define atomic_fadd(P, V) __sync_fetch_and_add((P), (V))

/* Atomic sub, returning the new value after the subtract */
#define atomic_subf(P, V) __sync_sub_and_fetch((P), (V))

/* Atomic sub, returning the value before the subtract */
#define atomic_fsub(P, V) __sync_fetch_and_sub((P), (V))

/* Force a read of the variable */
#define atomic_read(V) (*(volatile typeof(V) *)&(V))

/* Spin lock hint for the processor */
#define spinlock_hint() asm volatile("pause\n": : :"memory")

/* atomic exchange */
#define atomic_xchg(P, V) __sync_lock_test_and_set((P), (V))

/* atomic compare and exchange */
#define atomic_cmpxchg(P, OLD_V, NEW_V) \
	__sync_val_compare_and_swap((P), (OLD_V), (NEW_V))
/* atomic compare and exchange with bool returns */
#define atomic_bool_cmpxchg(P, OLD_V, NEW_V) \
	__sync_bool_compare_and_swap((P), (OLD_V), (NEW_V))

/* atomic "or", returning the new value after "or" */
#define atomic_orf(P,V) __sync_or_and_fetch((P), (V))

/* atomic "or", returning the new value before "or" */
#define atomic_for(P,V) __sync_fetch_and_or((P), (V))

/* atomic "and", returning the new value after "and" */
#define atomic_andf(P,V) __sync_and_and_fetch((P), (V))

/* atomic "and", returning the new value before "and" */
#define atomic_fand(P,V) __sync_fetch_and_and((P), (V))

/* A wrapper for the FUTEX system call; similar to the following function */
/* static long sys_futex(void *addr1, int op, int val1, struct timespec *timeout, void *addr2, int val3) */
#define sys_futex(addr1, op, val1, timeout, addr2, val3) \
	syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3)

/*
 * END: Atomic operations and other common definitions
 */

/*
 * BEGIN: fastsync barrier declarations
 */

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
	struct _fastsync_barrier *parent_bar; // pointer to the parent barrier,
                                              // used for tree-barrier
	int padding[11]; // make a barrier occupy a cache line to avoid false sharing
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
 * END: fastsync barrier declarations
 */


/*
 * BEGIN: fastsync mutex declarations
 */

typedef union _fastsync_mutex{	
	/* 
	 * each mutex occupies a whole cache line to avoid false sharing
	 */
	char padding[64]; 
	struct {
		/*
		 * state of the mutex:
		 * 0b00 (0): unlocked and un-contended
		 * 0b01 (1): locked and un-contended
		 * 0b10 (2): unlocked and contended
		 * 0b11 (3): locked and contended
		 *
		 * in short: last bit means locked or not, second to the last
		 * means contended or not
		 */
		int state;
		/*
		 * The owner id of a mutex node in the tree. If the node is a 
		 * core-level mutex, then this id is thread id. If the node is
		 * at processor-level, then this id is core id or node id.
		 * I add this owner id so that threads on the same core/node can
		 * take over a lock without deadlock on it. 
		 */
		int cur_owner;
		/*
		 * The thread owner id of parent (next) mutex. This thread owner
		 * id in conjunction with the cur_owner of the parent mutex, 
		 * uniquely identifies the owner of the parent mutex. 
		 * This thread owner id is used to prevent a race condition in 
		 * the unlocking and locking of the child and parent mutex.
		 * If thread A unlocks the child and wants to unlock the parent.
		 * However, before A unlock the parent, thread B (which has a 
		 * same core or node id as A) may kick in and grab the parent
		 * lock. If B grabs parent before A can release it, A should not
		 * unlock the parent. This thread owner id and the 
		 * owner_transfer_lock is then used for A and B to contend for
		 * the ownership parent lock. Please check the comments at
		 * mutex_unlock_interproc form more information.
		 *
		 * Although the thread owner field can be associated with the
		 * parent, I choose to save them with the child. This is because
		 * the parent is global, and updating/locking its fields require
		 * a remote shoot down of other copies of the parent. Therefore,
		 * I saved them on the child. Because this thread owner id is 
		 * only meaningful between threads with same core/node id, it is
		 * OK to save them on the child. Threads that are on different 
		 * cores/nodes wont use the thread owner id.
		 */
		int next_thr_owner;
		int next_owner_transfer_lock;
		/* parent mutex */
		union _fastsync_mutex *parent;
		int wakeup_seq;
	};
}fastsync_mutex;

typedef struct _fastsync_mutex_attr{
	fastsync_mutex *parent; /* parent mutex */
}fastsync_mutex_attr;


/*
 * Initialized a fastsync mutex object:
 * Input parameters:
 *     mutex: the mutex to initialized
 *     attr: mutex attributes
 * Return value:
 *     0: success
 *     1: mutex is NULL
 */
int fastsync_mutex_init(fastsync_mutex *mutex, const fastsync_mutex_attr *attr);

/*
 * Lock a fastsync mutex; block if lock not acquired. This function first lock
 * the mutex at core-level. If succeed, it proceeds to lock at higher levels.
 * Input parameters:
 *     mutex: the mutex to lock
 *     child: the child mutex which calls this parent mutex
 *     thr_id: thread id of the caller
 *     core_id: id of the running core of the caller
 * Return value:
 *     0: success
 *     1: mutex is NULL
 */
int fastsync_mutex_lock(fastsync_mutex *mutex, int thr_id, int core_id);
/*
 * inter-processor version of the mutex lock
 */ 
int fastsync_mutex_lock_interproc(fastsync_mutex *mutex, fastsync_mutex *child,
				  int thr_id, int core_idx);

/*
 * Unlock a fastsync mutex. This function first unlock the mutex at core level,
 * if there is any thread waiting on this core, the mutex is transferred to this
 * thread.
 * Input parameters:
 *     mutex: the mutex to lock
 *     child: the child mutex which calls this parent mutex
 *     thr_id: thread id of the caller
 *     core_id: id of the running core of the caller
 * Return value:
 *     0: success
 *     1: mutex is NULL
 *     2: error with futex
 */
int fastsync_mutex_unlock(fastsync_mutex *mutex, int thr_id, int core_id);
/*
 * Unlock a fastsync mutex at inter-processor level; threads waiting at this 
 * level is released first (even if another tree at different tree-node waits
 * for this mutex before anyone on this particular tree-node.
 */ 
int fastsync_mutex_unlock_interproc(fastsync_mutex *mutex, 
				    fastsync_mutex *child, int thr_id, 
				    int core_id);

/*
 * END: fastsync mutex declarations
 */

#endif
