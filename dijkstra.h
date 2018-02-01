#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <time.h>
#include <unistd.h>
#include "structure.h"
#include <limits.h>



typedef struct node {
    int id;
    int bandwidth;
    struct node * next;
} node_t;

struct Graph { 
    int V;    // No. of switchees
    // Pointer to an array containing
    // adjacency lists
    node_t *adj_edge;
};

int maxWidth(int width[], int sptSet[], int V, int *switches);
int** dijkstra(Tpg tpg);