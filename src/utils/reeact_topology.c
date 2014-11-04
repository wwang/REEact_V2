/*
 * Basic routine for parsing topology of a machine. Read the topology 
 * automatically from the Operating System with hwloc (tested with 
 * hwloc 1.10.0).
 * For simplicity of implementation, I assume that the processors in
 * a machine are identical, i.e., each socket has the same number of
 * nodes, and each node has the same number of cores.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <hwloc.h>

#include "reeact_utils.h"


/*
 * Get an object's ancestor at a particular depth.
 *
 * Input parameters:
 *     obj: the object whose parent to get
 *     root: root object of the tree
 *     anc_depth: the depth of the ancestor
 * Output parameters:
 *     anc: a pointer to the correct ancestor
 *
 * Return value:
 *     0: success
 *     1: anc is NULL
 *     2: no ancestor at specified level
 *     3: anc_depth is smaller then the input object
 */
int get_ancestor_by_depth(struct hwloc_obj *obj, unsigned int anc_depth, 
			  struct hwloc_obj **anc)
{
	int depth;

	if(anc == NULL){
		LOGERR("anc is NULL\n");
		return 1;
	}

	depth = obj->depth - anc_depth;
	*anc = obj;
	while(depth > 0){
		if(*anc != NULL)
			*anc = (*anc)->parent;
		else{
			LOGERR("Not ancestor at depth %d\n", anc_depth);
			return 2;
		}
		depth--;
	}
	
	return 0;
}

/*
 * Enumerate the children of a hwloc object at specified depth. Essentially a 
 * breakable DFS search.
 *
 * Input parameters:
 *     s: the search stack, caller should create it, but do not initialized it
 *     child_depth: the depth of child to search for
 * Output parameters:
 *     child: the next child at specified level; if *child == NULL, then no more
 *            children can be found
 *
 * Return value:
 *     0: success
 *     1: s is NULL
 *     2: child is NULL
 *     3: no more children to find
 */
#define TOPO_SEARCH_STACK_SIZE 16
struct topo_search_stack{
	struct hwloc_obj * objs[TOPO_SEARCH_STACK_SIZE]; // DFS search stack, 
	                                                 // 16 should be enough
	int children_pushed[TOPO_SEARCH_STACK_SIZE]; // the number of children 
                                                     // pushed
	int stack_size; // number of elements in stack
	int init; // whether the stack is initialized
};
inline struct hwloc_obj * pop_topo_stack(struct topo_search_stack *s)
{
	if(s->stack_size == 0)
		return NULL;
	s->stack_size--;
	return s->objs[s->stack_size];
}
inline void push_topo_stack(struct topo_search_stack *s, struct hwloc_obj * obj)
{
	if(s->stack_size == TOPO_SEARCH_STACK_SIZE){
		LOGERR("topology search stack full\n");
		return;
	}
	s->objs[s->stack_size] = obj;
	s->children_pushed[s->stack_size++] = 0;
	return;
}
inline struct hwloc_obj * get_topo_stack_top(struct topo_search_stack *s, 
					     int* children_pushed)
{
	if(s->stack_size == 0)
		return NULL;
	*children_pushed = s->children_pushed[s->stack_size-1];
	return s->objs[s->stack_size-1];
}
inline void inc_children_pushed(struct topo_search_stack *s)
{
	if(s->stack_size == 0)
		return;
	s->children_pushed[s->stack_size-1]++;
	return;
}

int enum_children_by_depth(struct topo_search_stack *s, struct hwloc_obj * root,
			   unsigned int child_depth, struct hwloc_obj **child)
{
	struct hwloc_obj * o;
	int pushed;

	if(s == NULL)
		return 1;
	if(child == NULL)
		return 2;

	*child = NULL;

	//DPRINTF("in enum s init is %d, child_depth %d\n", s->init, child_depth);
	if(s->init == 0){
		//DPRINTF("in init\n");
		// initialized the stack
		s->init = 1;
		s->stack_size = 0;
		push_topo_stack(s, root);
	}
	//DPRINTF("out init\n");
	while(s->stack_size != 0){
		o = get_topo_stack_top(s, &pushed);
		//		DPRINTF("o depth %d, index %d, pushed %d\n", o->depth, 
		//	o->os_index, pushed);
		if(o->depth == child_depth){
			*child = pop_topo_stack(s); // found one child
			
			return 0;
		}
		else{
			if(pushed == o->arity){
				// no more child to push
				pop_topo_stack(s);
			}
			else{
				// pushing a new child
				inc_children_pushed(s);
				push_topo_stack(s, o->children[pushed]);
			}
								  
		}
	}
	
	return 3;
}

/*
 * Get the Processor id of a core: find the HWLOC_OBJ_PU object of the core. 
 * Since SMT is not useful to my polices, I only locate the first logical 
 * id on this core
 */
struct hwloc_obj * get_topo_core_id(struct hwloc_obj * core)
{
	struct hwloc_obj *p = core;
	
	while(p != NULL){
		if(p->type == HWLOC_OBJ_PU)
			return p;
		else
			p = p->first_child;
	}
	
	return NULL;
}

/*
 * Get the processor topology of current machine
 */
int reeact_get_topology(int **nodes, int **cores, int *socket_cnt, 
			int *node_cnt, int *core_cnt)
{
	int ret_val;
	hwloc_topology_t topology;
	int soc_depth, node_depth, core_depth;
	unsigned i, j;
	struct hwloc_obj *obj, *child;
	struct topo_search_stack s;
		
	
	if(nodes == NULL || cores == NULL || socket_cnt == NULL ||
	   node_cnt == NULL || core_cnt == NULL){
		LOGERR("wrong parameter\n");
		return 1;
	}
	
	/* 
	 * determine topology with hwloc
	 */
	ret_val = hwloc_topology_init(&topology);
	if(ret_val != 0){
		LOGERR("initializing hwloc failed with error %d\n",
		       ret_val);
		return 2;
	}
	ret_val = hwloc_topology_load(topology);
	if(ret_val != 0){
		LOGERR("determining topology with hwloc failed with error %d\n",
		       ret_val);
		return 2;
	}

	/*
	 * determine the number of sockets, nodes and cores
	 */
	soc_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET); 
	*socket_cnt = hwloc_get_nbobjs_by_depth(topology, soc_depth);
	node_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_NODE); 
	*node_cnt = hwloc_get_nbobjs_by_depth(topology, node_depth);
	core_depth = hwloc_get_type_depth(topology, HWLOC_OBJ_CORE); 
	*core_cnt = hwloc_get_nbobjs_by_depth(topology, core_depth);
	DPRINTF("Socket count s %d, node count is %d, core count is %d\n", 
		*socket_cnt, *node_cnt, *core_cnt);
	DPRINTF("Socket depth %d, node depth %d, core depth %d\n",
		soc_depth, node_depth, core_depth);
	/*
	 * node depth larger than socket depth, in this case
	 * switch node and socket
	 */
	if(node_depth <= soc_depth){
		int temp = soc_depth;
		soc_depth = node_depth;
		node_depth = temp;
		temp = *node_cnt;
		*socket_cnt = *node_cnt;
		*node_cnt = temp;
	}

	/*
	 * allocate spaces for storing the nodes and cores info
	 */
	*nodes = (int*)calloc(*node_cnt, sizeof(int));
	*cores = (int*)calloc(*core_cnt, sizeof(int));
	if(*nodes == NULL || *cores == NULL){
		LOGERRX("unable to allocated spaces to store topology "
			"information: ");
		ret_val = 3;
		goto error;
	}
	
	/*
	 * compute the cores per node, and nodes per socket counts
	 */
	*core_cnt = (*core_cnt) / (*node_cnt);
	*node_cnt = (*node_cnt) / (*socket_cnt);
		
	/*
	 * parse node information
	 */
	for(i = 0; i < *socket_cnt; i++){
		obj = hwloc_get_obj_by_depth(topology, soc_depth, i);
		s.init = 0; // uninitialized it
		
		/*
		 * enumerate over the socket and find all nodes associated 
		 * with it
		 */
		j = 0;
		enum_children_by_depth(&s, obj, node_depth, &child);
		while(child != NULL){
			// save the node information
			(*nodes)[i * *node_cnt+j] = child->os_index;
			j++;
			// find next node
			enum_children_by_depth(&s, obj, node_depth, &child);
			
		}
	}

	/*
	 * parse core information
	 */
	for(i = 0; i < *node_cnt * *socket_cnt; i++){
		obj = hwloc_get_obj_by_depth(topology, node_depth, i);
		s.init = 0; // uninitialized it
		
		/*
		 * enumerate over the nodes and find all cores associated 
		 * with it
		 */
		j = 0;
		enum_children_by_depth(&s, obj, core_depth, &child);
		child = get_topo_core_id(child); // get logical processor id
		while(child != NULL){
			// save the core information
			(*cores)[i * *core_cnt+j] = child->os_index;
			j++;
			// find next core
			enum_children_by_depth(&s, obj, core_depth, &child);
			child = get_topo_core_id(child);
		}
	}
	
	// set the return value to 0 (success)
	ret_val = 0;
 error:
	// cleanup hwloc
	hwloc_topology_destroy(topology);
	

	return ret_val;
}
