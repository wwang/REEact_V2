/*
 * Header file for REEact. The internal data structure used by REEact is defined
 * here.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 *
 */

#ifndef __REEACT_FRAMEWORK_H__
#define __REEACT_FRAMEWORK_H__

/*
 * data structure representing the processor topology
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
 */
struct processor_topo{
	int socket_cnt;
	int node_cnt;
	int core_cnt;
	int *nodes;
	int *cores;
};

/*
 * internal data structure of REEact
 */
struct reeact_data{
	int pid;
	char *proc_name_short; // process name long
	char *proc_name_long; // process base name (without path)
	void *policy_data; // pointing to the user-specified policy data
	struct processor_topo topology; // processor topology
	
};

#endif
