#include "dijkstra.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int maxWidth(int width[], int sptSet[], int V, int *switches) {
    // Initialize min value
    int max = INT_MIN, max_index;
    int v;
    for (v = 0; v < V; ++v) {
	    if (!switches[v]) continue;
	    if (sptSet[v] == 0 && width[v] > max) {
			max = width[v], max_index = v;
			//printf("max_index = %d\n",v);
	    }
    }
    return max_index;
}

int** dijkstra(Tpg tpg) {
	int n = tpg.switch_num;
	node_t *nodeptr[n];
	node_t tmp[n];
	node_t *ptr;
	int **res = (int **)malloc(n * sizeof(int*));
	int i, j;
	for (i = 0; i < n; ++i) {
	    res[i] = (int *)malloc(n * sizeof(int));
	    nodeptr[i] = &tmp[i];
  	}
  	for (i = 0; i < n; ++i) {
  		for (j = 0; j < n; ++j) {
  			res[i][j] = 0;
   		}
  	}
	struct Graph graph;
	int *switches = tpg.switches_ptr;
	graph.adj_edge = (node_t *)malloc(n * sizeof(node_t));
	for (i = 0; i < n; ++i){
    	nodeptr[i]->next = graph.adj_edge+i;
  	}
	//printf("please\n");
	for (i = 0; i < tpg.edge_num; ++i) {
	    int node1 = tpg.edge[i].node1 - 1;
	    int node2 = tpg.edge[i].node2 - 1;
	    //printf("node2 = %d", node2);
	    int bandwidth = tpg.edge[i].bandwidth;
	    int active = tpg.edge[i].active;
	    //printf("switches %d %d %d\n", switches[node1], switches[node2], active);
	    if (switches[node1] && switches[node2] && active) {
			//printf("why\n");
			if (nodeptr[node1]->next == NULL) nodeptr[node1]->next = (node_t *)malloc(sizeof(node_t));
			//tmp = nodeptr+node1;
			//tmp = tmp->next;
			//printf("why\n");
			nodeptr[node1] = nodeptr[node1]->next;
			//printf("why\n");
			nodeptr[node1]->id = node2;
			//printf("node1->id = %d\n", graph.adj_edge->id);
			//printf("why\n");
			nodeptr[node1]->bandwidth =  bandwidth;
			nodeptr[node1]->next = NULL;
			if (nodeptr[node2]->next == NULL) nodeptr[node2]->next = (node_t *)malloc(sizeof(node_t));
			//tmp = nodeptr+node2;
			//tmp = tmp->next;
			nodeptr[node2] = nodeptr[node2]->next;
			nodeptr[node2]->id = node1;
			nodeptr[node2]->bandwidth =  bandwidth;
			nodeptr[node2]->next = NULL;
    	}
  	}
	/*ptr = graph.adj_edge+2;
	while (ptr != NULL) {
	printf("test id = %d\n",ptr->id);
	ptr = ptr->next;
	}*/
	int src;
	for (src = 0; src < n; ++src) {
	    //printf("scr = %d\n", src);
	    if (!switches[src]) continue;
	    int width[n];
	    int sptSet[n];
	    for (i = 0; i < n; ++i)
        	width[i] = -1, sptSet[i] = 0;
	    width[src] = INT_MAX;
	    int count;
	    int prev[n];
	    for (count = 0; count < n; ++count) {
			//printf("in\n");
			// Pick the minimum distance vertex from the set of vertices not
			// yet processed. u is always equal to src in first iteration.
			int u = maxWidth(width, sptSet, n, switches);
			//printf("%d\n",u);
			// Mark the picked vertex as processed
			sptSet[u] = 1;

			if (width[u] == -1) break;
			if (u != src) {
		        if (prev[u] == src) res[src][u] = u + 1;
		        else res[src][u] = res[src][prev[u]];
      		}

			// Update dist value of the adjacent vertices of the picked vertex.
			ptr = graph.adj_edge+u;
			while (ptr != NULL) {
		        //printf("hwihaoi\n");
		        int v = ptr->id;
		        //printf("yes\n");
		        if (!sptSet[v] && (MIN(width[u], ptr->bandwidth) > width[v])) {
		            width[v] = MIN(width[u], ptr->bandwidth);
		            prev[v] = u;
        		}
		        //printf("id = %d\n",ptr->id);
		        ptr = ptr->next;
		        //if (ptr == NULL) printf("GG\n");
		        //printf("no\n");
      		}
			//printf("width = %d %d %d\n", width[0],width[1],width[2]);
			//printf("what\n");

    		}
  	}
	for (i = 0; i < n; i++) {
    	for (j = 0; j < n; j++) {
        	printf("%d ", res[i][j]);
    	}
    	printf("\n");
  	}

  	for (i = 0; i < n; i++) {
	    nodeptr[i] = graph.adj_edge+i;
	    if (nodeptr[i]->next != NULL) continue;
	    nodeptr[i] = nodeptr[i]->next;
	    while(nodeptr[i] != NULL) {
			ptr = nodeptr[i];
			nodeptr[i] = nodeptr[i]->next;
			//printf("noteptr[i] = %d\n", nodeptr[i]->id);
			//printf("1\n");
			free(ptr);
			//printf("2\n");
    	}
  	}	
  	free(graph.adj_edge);
	//printf("3\n");
  	return res;
}
