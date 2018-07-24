/* mapconst.h -- constants for map.h */

#define NUMBER_NODES 137
#define NUMBER_LINES 316
#define MAX_NODE_LINES 4
#define NODES_IN_UPDATE 40
#define min_val(a,b) (a<b ? a:b)
#define QUERY_LINES min_val(NUMBER_LINES,MAX_NODE_LINES*NODES_IN_UPDATE)
