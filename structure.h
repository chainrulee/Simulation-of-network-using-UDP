#ifndef STRUCTURE_H
#define STRUCTURE_H
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
#endif
