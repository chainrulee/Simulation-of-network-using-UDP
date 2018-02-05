#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include "structure.h"
#define CLOCKID CLOCK_REALTIME

void timer_thread(union sigval v);

// create the function to be executed as a thread
void *thread(void *ptr)
{
    int type = (int) ptr;
	 while (1) {
        printf(" Main switch funciton \n");
        sleep(5);
     }

    return  ptr;
}
void process_response(char buf[]);
void process_router_update(char buf[]);
void process_keep_alive(char buf[]);
void periodic_tpg_update_thread(union sigval v);
void periodic_send_keep_alive(union sigval v) ;

volatile static int linkk;
volatile fd_set rfds;
volatile struct timeval tv;
volatile int retval;
volatile int recvlen;
volatile int fd;
volatile int self_id, total_number, neighbor_number; 
//  next_hop[i]: If we want to send pkt to node i, the next hop
//               for this pkt is next_hop[i] 
//
//  next_hop[i]:  start from i = 0 to total_number
//  neighbors[i]: start from i = 1 to neighbor_number+1
Nbor *next_hop = NULL, *neighbors = NULL;
struct sockaddr_in myaddr, remaddr;
volatile timer_t* tpg_timerid;

int main(int argc, char **argv)
{
    // create the thread objs
    pthread_t thread1;
    int thr = 1;
	socklen_t addrlen = sizeof(remaddr);
	char *server = "127.0.0.1";
	int i;
	for (i = 0; i < argc; ++i) {
		printf("argv[%d] is %s @%s \n", i, argv[i], __FUNCTION__);
	}
	self_id = atoi(argv[1]);
	/* create a socket */

	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");

	/* bind it to all local addresses and pick any port number */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(SERVICE_PORT);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
	//========================================================//
	printf("Sending packet %d to %s port %d\n", i, server, SERVICE_PORT);
    pid_t send;
	if (send = fork() == 0) {
		char buf[BUFSIZE];
        buf[0] = REGISTER_REQUEST;
	    buf[1] = self_id;
		struct sockaddr_in remaddr_l = remaddr;
		remaddr_l.sin_port = htons(SERVICE_PORT);
        if (sendto(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr_l, addrlen)==-1) {
            perror("sendto");
	    }
		exit(EXIT_SUCCESS);
    } else if (send < 0) {
        printf("Fork to Send REGISTER_REQUEST failed! \n");
	}
	
	
	// pthread_create(&thread1, NULL, *thread, (void *) thr);
    while (1) {
		char rcv_buf[BUFSIZE];
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		FD_SET(STDIN_FILENO, &rfds);
		retval = select(fd+1, &rfds, NULL, NULL, NULL);
		if (FD_ISSET(STDIN_FILENO, &rfds)) {
		    read(STDIN_FILENO, &linkk, sizeof(int));
		    printf("children's pthread, got number: %d \n", linkk);
		} else if (FD_ISSET(fd, &rfds)) {
		    recvlen = recvfrom(fd, rcv_buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
			printf("Recieve buf[0] is %d \n", rcv_buf[0]);
			rcv_buf[recvlen] = 0;
			switch(rcv_buf[0]) {
			    case REGISTER_RESPONSE:
					process_response(rcv_buf);
				    break;
			    case ROUTER_UPDATE:
				    process_router_update(rcv_buf);
				    break;
			    case KEEP_ALIVE:
                    process_keep_alive(rcv_buf);
                    break;
                default:
				    printf("Unknown packet! header number: %d \n", rcv_buf[0]);
                    break;				
			
			}
		}
        
     }
    return 0;
}

void process_keep_alive(char buf[]) {
    int nid = buf[1];
	int i, idx = -1;
	for (i = 0; i < neighbor_number; ++i) {
	    if (neighbors[i].nid == nid) {
		    idx = i;
            break;			
		}
	}
	if (idx < 0) {
	    printf("idx: %d < 0, error! \n");
		return;
	}
	if (neighbors[idx].active == 0) {
	    // send TPG_UPDATE pkt
	} else {
	    //=== reset the timer ===//
	    timer_t* timerid = neighbors[idx].timerid;
	    struct itimerspec it;
	    it.it_interval.tv_sec = 6;	//间隔6s
        it.it_interval.tv_nsec = 0;
        it.it_value.tv_sec = 6;		
        it.it_value.tv_nsec = 0;

	    if (timer_settime(*timerid, 0, &it, NULL) == -1) {
		    perror("fail to timer_settime");
		    exit(-1);
	    }
	}
}

void process_router_update(char buf[]) {
    int i;
	// buf[0]: ROUTER_UPDATE
	// buf[1]: neibor id
	// ...
	for (i = 1; i <= total_number; ++i) {
	    next_hop[i].nid = buf[i];
	}
}

void process_response(char buf[]) {
    total_number = buf[1];
	neighbor_number = buf[2];
	printf("We got REGISTER_RESPONSE, total_number: %d, neighbor_number: %d \n", total_number, neighbor_number);
	neighbors = (Nbor*) malloc(neighbor_number*sizeof(Nbor));
	next_hop = (Nbor*) malloc((total_number+1)*sizeof(Nbor));
	int i;
	for (i = 0; i < neighbor_number; ++i) {
		neighbors[i].nid = buf[3+4*i];
		neighbors[i].active = buf[3+4*i+1];
		neighbors[i].port = (buf[3+4*i+2] << 8) | buf[3+4*i+3];
		printf("3+4*i: %d, nid: %d, active: %d, port: %d \n", 3+4*i, buf[3+4*i], neighbors[i].active, neighbors[i].port);
		timer_t* timerid = neighbors[i].timerid= (timer_t*) malloc(sizeof(timer_t));
		tpg_timerid = (timer_t*) malloc(sizeof(timer_t));
		//=== Setup timer for monitoring KEEP_ALIVE ===//
		struct sigevent evp;
	    memset(&evp, 0, sizeof(struct sigevent));

	    evp.sigev_value.sival_int = neighbors[i].nid;
	    evp.sigev_notify = SIGEV_THREAD;

	    //=== timeout timer ===//
		evp.sigev_notify_function = timer_thread;
	    if (timer_create(CLOCKID, &evp, timerid) == -1) {
		    perror("fail to timer_create");
		    exit(-1);
	    }

        //=== tpg periodic update ===//
		evp.sigev_notify_function = periodic_tpg_update_thread;
	    if (timer_create(CLOCKID, &evp, tpg_timerid) == -1) {
		    perror("fail to timer_create");
		    exit(-1);
	    }

		
	    struct itimerspec it;
	    it.it_interval.tv_nsec = 0;		
	    it.it_value.tv_nsec = 0;

	    it.it_value.tv_sec =  6;
		it.it_interval.tv_sec = 6;
	    if (timer_settime(*timerid, 0, &it, NULL) == -1) {
		    perror("fail to timer_settime");
		    exit(-1);
	    }

		it.it_interval.tv_sec = 3;	//间隔3s
        it.it_value.tv_sec = 3;
		if (timer_settime(*tpg_timerid, 0, &it, NULL) == -1) {
		    perror("fail to timer_settime");
		    exit(-1);
	    }
	}
}

void periodic_send_keep_alive(union sigval v) {
    //=== send KEEP_ALIVE periodically ===//
	char buf[BUFSIZE];
    buf[0] = KEEP_ALIVE;
	buf[1] = self_id;
	socklen_t addrlen = sizeof(remaddr);
	struct sockaddr_in remaddr_l = remaddr;
	int i;
	for (i = 0; i < neighbor_number; ++i) {
        remaddr_l.sin_port = neighbors[i].port;
        if (sendto(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr_l, addrlen)==-1) {
            printf("sendto, %s()", __FUNCTION__);
	    }
	}	
}

void periodic_tpg_update_thread(union sigval v) {
    //=== send TPG update periodically ===//
	char buf[BUFSIZE];
    buf[0] = TPG_UPDATE;
	buf[1] = self_id;
	socklen_t addrlen = sizeof(remaddr);

	int i, j = 3, live_num = 0;
	for (i = 0; i < neighbor_number; ++i) {
	    if (neighbors[i].active == 1) {
			buf[j++] = neighbors[i].nid;
			++live_num;
		}
	}
	buf[2] = live_num;
	struct sockaddr_in remaddr_l = remaddr;
	remaddr_l.sin_port = htons(SERVICE_PORT);
    if (sendto(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr_l, addrlen)==-1) {
            perror("sendto");
	}
}

void timer_thread(union sigval v)
{
	printf("@%s function! nid: %d is timeout. \n", __FUNCTION__, v.sival_int);
//=== send TPG update since timeout ===//
	int monitor_nid = v.sival_int;
	char buf[BUFSIZE];
    buf[0] = TPG_UPDATE;
	buf[1] = self_id;
	socklen_t addrlen = sizeof(remaddr);

	int i, j = 3, live_num = 0, idx = -1;
	for (i = 0; i < neighbor_number; ++i) {
	    if (neighbors[i].active == 1) {
			buf[j++] = neighbors[i].nid;
			++live_num;
		}
		if (neighbors[i].nid == monitor_nid) {
		    idx = i;
		}
	}
	if (idx > -1) {
		neighbors[idx].active = 0;
	}
	buf[2] = live_num;
	struct sockaddr_in remaddr_l = remaddr;
	remaddr_l.sin_port = htons(SERVICE_PORT);
    if (sendto(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr_l, addrlen)==-1) {
            perror("sendto");
	}
	//=== disable the timer ===//
	timer_t* timerid = neighbors[idx].timerid;
	struct itimerspec it;
	it.it_interval.tv_sec = 0;	//间隔5s
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = 0;		
	it.it_value.tv_nsec = 0;

	if (timer_settime(*timerid, 0, &it, NULL) == -1)
	{
		perror("fail to timer_settime");
		exit(-1);
	}
	
}