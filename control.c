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
#include <signal.h>
#define DEBUG 0
#define CLOCKID CLOCK_REALTIME

//Global Variable
Tpg tpg;
timer_t **timerid;
struct sigevent evp;
struct itimerspec it;
unsigned char buf[BUFSIZE]; /* receive buffer */
int** nextHop;
int *port;
struct sockaddr_in remaddr; /* remote address */
int slen=sizeof(remaddr);
int n;
int fd;             /* our socket */

node_t* assignAdjEdge(Tpg tpg) {
    n = tpg.switch_num;
    port = (int *)malloc(sizeof(int)*n);
    timerid = (timer_t **)malloc(sizeof(timer_t *)*n);
    node_t *nodeptr[n];
    node_t tmp[n];
    int i;
    for (i = 0; i < n; ++i) {
        nodeptr[i] = &tmp[i];
    }
    struct Graph graph;
    int *switches = tpg.switches_ptr;
    graph.adj_edge = (node_t *)malloc(n * sizeof(node_t));
    for (i = 0; i < n; ++i){
        nodeptr[i]->next = graph.adj_edge+i;
    }
    for (i = 0; i < tpg.edge_num; ++i) {
        int node1 = tpg.edge[i].node1 - 1;
        int node2 = tpg.edge[i].node2 - 1;
        int bandwidth = tpg.edge[i].bandwidth;
        int active = tpg.edge[i].active;
        //printf("in\n");
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
    return graph.adj_edge;
}


void timer_thread(union sigval v)
{
    int i,j;
    printf("Time limit exceeded! Switch %d is dead\n", v.sival_int + 1);
    evp.sigev_value.sival_int = v.sival_int;
    it.it_interval.tv_sec = 0;  //间隔5s
    it.it_value.tv_sec = 0;
    if (timer_settime(*timerid[v.sival_int], 0, &it, NULL) == -1)
    {
        perror("fail to timer_settime");
        exit(-1);
    }
    *(tpg.switches_ptr+v.sival_int) = 0;
    printf("Sending ROUTER_UPDATE (switch is down)\n");
    nextHop = dijkstra(tpg);
    //printf("what about here\n");
    buf[0] = ROUTER_UPDATE;
    for (i = 0; i < n; i++) {
        if (*(tpg.switches_ptr+i) == 1) {
            //printf("Sending ROUTER_UPDATE to switch %d\n", i+1);
            for (j = 0; j < n; j++) {
                buf[j+1] = nextHop[i][j];                                
            }
            remaddr.sin_port = *(port+i);
            if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1)
                perror("ROUTER_UPDATE");
        }
    }
}

void control_process() {
    struct sockaddr_in myaddr;  /* our address */
    socklen_t addrlen = sizeof(remaddr);        /* length of addresses */
    int recvlen;            /* # bytes received */
    node_t *ptr;
    int i, j, id;

    //=====//
    fd_set rfds;
    struct timeval tv;
    int retval;
    //=====//
    /* create a UDP socket */

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return;
    }

    /* bind the socket to any valid IP address and a specific port */

    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(SERVICE_PORT);

    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        perror("bind failed");
        return;
    }

    //create a map to adjacent edges
    node_t* adj_edge = assignAdjEdge(tpg);
    int alive[n][n];
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            alive[i][j] = 0;
        }
    }
    char hostname[n][BUFSIZE];
    //create a table for storing next hops
    pid_t pid;

    //timer 
    memset(&evp, 0, sizeof(struct sigevent));   

    //evp.sigev_value.sival_int = 1;        
    evp.sigev_notify = SIGEV_THREAD;        
    evp.sigev_notify_function = timer_thread;   

    it.it_interval.tv_nsec = 0;
    it.it_value.tv_nsec = 0;

    /* now loop, receiving data and printing what we received */
    while (1) {
        //printf("waiting on port %d\n", SERVICE_PORT);
        //tv.tv_sec = 5; tv.tv_usec = 0; /* Wait up to five seconds. */
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        retval = select(fd+1, &rfds, NULL, NULL, NULL);
        if (FD_ISSET(fd, &rfds)) {
            recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
            if (recvlen > 0) {
                buf[recvlen] = 0;
                //printf("received message: \"%s\"\n", buf);
                switch(buf[0]) {
                    case REGISTER_REQUEST:
                        printf("Received REGISTER_REQUEST from %d\n", buf[1]);
                        id = buf[1]-1;
                        strcpy(hostname[id],buf+2);
                        printf("hostname: %s\n", hostname[id]);
                        *(tpg.switches_ptr+id) = 1;
                        *(port+id) = remaddr.sin_port;
                        //printf("switch %d has port %d\n", id+1, *(port+id));
                        buf[0] = REGISTER_RESPONSE;
                        ptr = adj_edge+id;
                        i = 3;
                        int cnt = 0;
                        //int mask1 = 0xFF00;
                        int mask2 = 0xFF;
                        while (ptr != NULL) {
                            //printf("i = %d\n", i);
                            buf[i++] = ptr->id + 1;
                            //printf("found neighbor id = %d\n", buf[i-1]);
                            buf[i++] = *(tpg.switches_ptr+ptr->id);
                            //printf("port[%d] = %d\n", ptr->id, *(port+ptr->id));
                            buf[i++] = *(port+ptr->id) >> 8;
                            //printf("port first part = %d\n", buf[i-1]);
                            buf[i++] = *(port+ptr->id) & mask2;
                            //printf("port sencond part = %d\n", buf[i-1]);
                            ptr = ptr->next;
                            ++cnt;
                        }
                        buf[i] = '\0';
                        ptr = adj_edge+id;
                        while (ptr != NULL) {
                            if (*(tpg.switches_ptr+ptr->id) == 0) {
                                ptr = ptr->next;
                                continue;
                            }
                            strcat(buf,hostname[ptr->id]);
                            strcat(buf," ");
                            ptr = ptr->next;
                        }
                        buf[1] = n;
                        buf[2] = cnt;
                        //===== send to response to client =====//
                        printf("Sending REGISTER_RESPONSE\n");
                        if (sendto(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, slen)==-1)
                            perror("REGISTER_RESPONSE");                    
                        pid = fork();
                        if (pid == 0) {
                            //NOW WE HAVE TO SEND ROUTER UPDATE TO EVERY OTHER SWITCHES
                            printf("Sending ROUTER_UPDATE (switch is registered\n");
                            nextHop = dijkstra(tpg);
                            //printf("what about here\n");
                            buf[0] = ROUTER_UPDATE;
                            for (i = 0; i < n; i++) {
                                if (*(tpg.switches_ptr+i) == 1) {
                                    //printf("Sending ROUTER_UPDATE to switch %d\n", i+1);
                                    for (j = 0; j < n; j++) {
                                        buf[j+1] = nextHop[i][j];                                
                                    }
                                    remaddr.sin_port = *(port+i);
                                    if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1)
                                        perror("ROUTER_UPDATE");
                                }
                            }
                            //printf("I am here\n");
                            //kill(getpid(), SIGKILL);
                            printf("ROUTER_UPDATE sent\n");
                            exit(EXIT_SUCCESS);
                        }
                        timerid[id] = (timer_t *)malloc(sizeof(timer_t));
                        //Set timer to monitor TPG_UPDATE
                        it.it_interval.tv_sec = 5;  //间隔5s
                        it.it_value.tv_sec = 5;
                        evp.sigev_value.sival_int = id;   
                        if (timer_create(CLOCKID, &evp, timerid[id]) == -1)
                        {
                            perror("fail to timer_create");
                            exit(-1);
                        }
                        if (timer_settime(*timerid[id], 0, &it, NULL) == -1)
                        {
                            perror("fail to timer_settime");
                            exit(-1);
                        }
                        break;
                    case TPG_UPDATE:
                        printf("Received TPG_UPDATE from %d\n", buf[1]);
                        int id = buf[1] - 1;
                        it.it_interval.tv_sec = 5;  //间隔5s
                        it.it_value.tv_sec = 5;
                        evp.sigev_value.sival_int = id;
                        if (timer_settime(*timerid[id], 0, &it, NULL) == -1)
                        {
                            perror("fail to timer_settime");
                            exit(-1);
                        }
                        int change[n];
                        int isChange = 0;
                        for (i = 0; i < n; ++i) {
                            change[i] = 0;
                        }
                        for (i = 0; i < buf[2]; ++i) {
                            change[buf[3+i]-1] = 1;
                            printf("alive id from %d: %d\n", id+1, buf[3+i]);
                        }
                        ptr = adj_edge+id;
                        while (ptr != NULL) {
                            if (alive[id][ptr->id] != change[ptr->id]) {
                                if (!change[ptr->id]) printf("Link failure: %d -> %d\n", id+1, ptr->id + 1);
                                else printf("Link reconnected: %d -> %d\n", id+1, ptr->id + 1);
                                isChange = 1;
                                alive[id][ptr->id] = change[ptr->id];
                                if (id < ptr->id) {
                                    for (i = 0; i < tpg.edge_num; ++i) {
                                        if (tpg.edge[i].node1 - 1 == id && tpg.edge[i].node2 - 1 == ptr->id) {
                                            tpg.edge[i].active = change[ptr->id];
                                            break;
                                        }
                                    }
                                }
                                else {
                                    for (i = 0; i < tpg.edge_num; ++i) {
                                        if (tpg.edge[i].node2 - 1 == id && tpg.edge[i].node1 - 1 == ptr->id) {
                                            tpg.edge[i].active = change[ptr->id];
                                            break;
                                        } 
                                    }
                                }
                            }
                            ptr = ptr->next;
                        }
                        if (isChange) {
                            /*for (i = 0; i < n; ++i){
                                for (j = 0; j < n; ++j) {
                                    printf("alive[%d][%d] = %d\n", i, j, alive[i][j]);
                                }
                            }*/
                            pid = fork();
                            if (pid == 0) {
                                printf("Sending ROUTER_UPDATE (change of link status)\n");
                                nextHop = dijkstra(tpg);
                                buf[0] = ROUTER_UPDATE;
                                for (i = 0; i < n; i++) {
                                    if (*(tpg.switches_ptr+id) == 1) {
                                        for (j = 0; j < n; j++) {
                                            buf[j+1] = nextHop[i][j];                                
                                        }
                                        remaddr.sin_port = *(port+i);
                                        if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1)
                                            perror("ROUTER_UPDATE");
                                    }
                                }
                                exit(EXIT_SUCCESS);
                            }
                        }
                        break;
                }
            }
        } else {
            printf("Retval: %d; No data within five seconds.\n",retval);
        }
    }

    //free memory
    node_t *nodeptr[n];
    for (i = 0; i < n; i++) {
        nodeptr[i] = adj_edge+i;
        nodeptr[i] = nodeptr[i]->next;
        while(nodeptr[i] != NULL) {
            ptr = nodeptr[i];
            nodeptr[i] = nodeptr[i]->next;
            free(ptr);
        }
    }
    free(adj_edge);  
}

int main(int argc, char **argv) {
    FILE *fptr;
    char login_info[128];
    char *token;
    
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
	    *(tpg.switches_ptr+cnt) = 0;
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
    control_process(); 
    free (tpg.edge);
    free (tpg.switches_ptr);
    return 0;
}
