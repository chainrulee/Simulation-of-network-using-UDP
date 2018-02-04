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
#include "dijkstra.h"
#define DEBUG 1

int main(int argc, char **argv) {
    FILE *fptr;
    char login_info[128];
    char *token;
    Tpg tpg;
    
    if ((fptr = fopen(argv[1], "r")) == NULL) {
    printf("open_file_error");
        exit(1);
    }

    int cnt = 0;
    while(fgets(login_info, 128, fptr) != NULL) {
    /* get the first token */
        token = strtok(login_info, "\n");
        /* walk through other tokens */
        while( token != NULL ) {
           ++cnt;
           token = strtok(NULL, "\n");
        }
    }
	tpg.edge_num = cnt-1;
    tpg.edge = (Edge*)malloc(tpg.edge_num*sizeof(Edge));
//    printf("line numbers: %d \n", cnt);
    fclose(fptr);

    if ((fptr = fopen(argv[1], "r")) == NULL) {
        printf("open_file_error");
        exit(1);
    }
	tpg.switch_num = -1;
    int edge_idx = 0;
    while(fgets(login_info, 128, fptr) != NULL) {
        /* get the first token */
        token = strtok(login_info, " \n");

        /* walk through other tokens */
        int cur = 0;
        while( token != NULL ) {
		   if (tpg.switch_num == -1) {
		       tpg.switch_num = atoi(token);
			   token = strtok(NULL, " \n");
			   edge_idx = -1;
			   continue;
		   }

           switch (cur++) {
               case 0:
                   (tpg.edge+edge_idx)->node1 = atoi(token);
                   break;
			   case 1:
                   (tpg.edge+edge_idx)->node2 = atoi(token);
                   break;
               case 2:
                   (tpg.edge+edge_idx)->bandwidth = atoi(token);
                   break;
               case 3:
                   (tpg.edge+edge_idx)->delay = atoi(token);
                   (tpg.edge+edge_idx)->active = 1;
                   break;
               default:
                   break;
		   }
#if DEBUG == 1
           printf( "%d\n", atoi(token));
#endif		   
           token = strtok(NULL, " \n");
        }
        ++edge_idx;
    }
	fclose(fptr);
	tpg.switches_ptr = (int*)malloc(tpg.switch_num*sizeof(int));
	for (cnt = 0; cnt < tpg.switch_num; ++cnt) {
	    *(tpg.switches_ptr+cnt) = 1;
	}
#if DEBUG == 1
	for (cnt = 0; cnt < tpg.switch_num; ++cnt) {
	    printf("%d \n", *(tpg.switches_ptr+cnt));
	}
	for (cnt = 0; cnt < tpg.edge_num; ++cnt) {
		printf("edge[%d], node1: %d, node2: %d, bandwidth: %d, delay: %d, edge_num: %d, switch_num: %d \n ",
		cnt, tpg.edge[cnt].node1, (tpg.edge+cnt)->node2, (tpg.edge+cnt)->bandwidth, (tpg.edge+cnt)->delay, tpg.edge_num, tpg.switch_num);
    }
#endif
  int** j = dijkstra(tpg);
	free (tpg.edge);
    return 0;
}
