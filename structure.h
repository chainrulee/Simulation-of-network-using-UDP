#ifndef STRUCTURE_H
#define STRUCTURE_H

#define SERVICE_PORT 21234
#define REGISTER_REQUEST 246 
#define REGISTER_RESPONSE 61
#define KILL_SWITCH 62
typedef struct edge {
	int node1;
	int node2;
	int bandwidth;
	int delay;
	int active;
	
} Edge;


typedef struct topology {
	Edge *edge;
	int edge_num;
	int switch_num;
	int *switches_ptr;
} Tpg;

typedef struct pair_pid_nid {
	pid_t pid;
	int nid;
	int active;
	int *pipe_fd_p;
} PNid;
#endif
