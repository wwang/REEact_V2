/*
 * Utility functions for REEact.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __REEACT_UTILITY_H__
#define __REEACT_UTILITY_H__

/* 
 * Compilation time controlled debug output
 */
#ifdef _REEACT_DEBUG_
#define DPRINTF(fmt, ...)					\
	do { reeact_dprintf("%s: "fmt , __func__,		\
			    ## __VA_ARGS__);} while (0);
#else
#define DPRINTF(fmt, ...)			\
	do {} while(0);
#endif

/*
 * Execution time controlled debug output
 */
#define dprintf(debug,fmt, ...)						\
	do { if(debug) {reeact_dprintf("%s: "fmt , __func__,		\
				       ## __VA_ARGS__);}} while (0);

/*
 * Error logging without system error message
 */ 
#define LOGERR(fmt,...)							\
	do{ reeact_log_err("%s: "fmt, __func__,				\
			   ## __VA_ARGS__);} while (0);

/*
 * Error logging with system error message
 */ 
#define LOGERRX(fmt,...)						\
	do{ reeact_log_errx("%s: "fmt, __func__,			\
			    ## __VA_ARGS__);} while (0);


/*
 * Normal debug logging to stderr output. Return value is the same as vfprintf.
 *
 */
int reeact_dprintf(const char* fmt, ...);

/*
 * Error logging without append system error message. Return value same as 
 * vfprintf.
 */
int reeact_log_err(const char *fmt, ...);

/*
 * Error logging with system error message appended. Return values same as
 * fprintf.
 */
int reeact_log_errx(const char *fmt, ...);


/*
 * Determine the processor topology of current machine, i.e., the sockets, 
 * nodes and cores.
 * Input parameters:
 *
 * Output parameters:
 *    nodes: 2D array of node ids, indexed by socket id and node index within 
 *           the socket, e.g., if node 3 is the second node on socket 0, then 
 *           nodes[0][1] = 3; 2D array is flattened for access
 *           array allocated by this functions, caller should de-allocate it.
 *    cores: 2D array of core ids, indexed by node id and core index on within
 *           the node, e.g., if core 10 is the fourth core on node 4, then
 *           cores[4][3] = 10; 2D array is flattened for access
 *    socket_cnt: the number of sockets
 *    node_cnt: the number of nodes per socket
 *    core_cnt: the number of cores per node
 * Return value:
 *    0: success
 *    1: one of the output parameter is NULL
 *    2: error reading topology
 *    3: error allocating space
 *
 */
int reeact_get_topology(int **nodes, int **cores, int *socket_cnt, 
			int *node_cnt, int *core_cnt);
/*
 * user configure file with topology information
 */
#define REEACT_USER_TOPOLOGY_CONFIG "REEACT_TOPO_CONFIG"

#endif
