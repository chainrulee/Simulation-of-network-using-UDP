#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include "structure.h"
/***
Usage: ./main [topology] file. Example:./main topology.txt

Start a switch: switch [id] [hostname] [port] [-s/-k]
-k: kill a switch process
-s: activate a switch process

***/
int main (int argc, char **argv) {
	FILE *fptr;
	char login_info[128];
	char hostname[128];

	if ((fptr = fopen(argv[1], "r")) == NULL) {
        printf("open_file_error");
        exit(1);
    }
	pid_t ctrl_pid;
	ctrl_pid = fork();
	if (ctrl_pid == -1) {
		perror("fork failure");
		exit(EXIT_FAILURE);
	} else if (ctrl_pid == 0) {
		execl("control", "control", argv[1], NULL);
		perror("Shoud not go here. this line means execl somewhat failed! \n");
	}

	fgets(login_info, 128, fptr);
	int node_num = atoi(strtok(login_info, "\n"));
	if (node_num < 1) {
		printf("Fail to get switch numbers from topology file! \n");
		exit(1);
	}
	PNid *table = (PNid*) malloc(node_num* sizeof(PNid));
	int i;
	for (i = 0; i < node_num; ++i) {
		table[i].pid = -1;
		table[i].nid = table[i].active = 0;
	}
	gethostname(hostname, 128);
	printf("hostname is %s \n", hostname);
	//	printf("The number of nodes is %d \n", node_num);
	fd_set rfds;
	struct timeval tv;
	int retval;
	char ch1,ch2;

	pid_t pid, pid2;

	while(1) {
		tv.tv_sec = 5; tv.tv_usec = 0; /* Wait up to five seconds. */
		FD_ZERO(&rfds);
		FD_SET(0, &rfds);
		retval = select(1, &rfds, NULL, NULL, NULL);

		if (FD_ISSET(0, &rfds)) {
			char whole_cmd [256] = {' '};
			char *cmd [6];
			fgets(whole_cmd, 256, stdin);
			cmd[0] = strtok(whole_cmd, " \n");

			int i;
			for (i = 1; cmd[i-1] != NULL && i < 6; ++i) {
				cmd[i] = strtok(NULL, " \n");
			}
			if (i < 6) {
				printf("Error! numbers of argument: %d, not enought, please enter again. \n", i);
				continue;
			}
/*
			int id = atoi(cmd[1]);
			int port = atoi(cmd[3]);
*/
			int nid = atoi(cmd[1]);
			if (strcmp(cmd[4], "-f") == 0) { // equal
				if (table[nid].active == 0)
					continue;

				int *pipe_fd = table[nid].pipe_fd_p;
				write(pipe_fd[1], &nid, sizeof(int));

			} else {
				if (strcmp(cmd[4], "-s") == 0) {
					if (table[nid].active == 1)
						continue;
						//===== Prepare Pipe =====//
					int *pipe_fd = table[nid].pipe_fd_p ;
					if (pipe(pipe_fd) == -1) {
						printf("Error: Unable to create pipe. \n");
						exit(EXIT_FAILURE);
					}

					pid = fork();
					if (pid == -1) {
						perror("fork failure");
						exit(EXIT_FAILURE);
					} else if (pid == 0){
						printf("pid in child=%d and parent=%d\n",getpid(),getppid()); 
						close(pipe_fd[1]);
						dup2(pipe_fd[0], STDIN_FILENO);
						close(pipe_fd[0]);
						execl("switch", "switch", cmd[1], cmd[2], cmd[3], NULL);
					} else {
						printf("pid in parent=%d and childid=%d\n",getpid(),pid);
						pid2 = pid;
						close(pipe_fd[0]);
					}
					table[nid].nid = nid;
					table[nid].pid = pid2;
					table[nid].active = 1;
				} else if (strcmp(cmd[4], "-k") == 0) {
					if (table[nid].active == 0)
						continue;
					kill(table[nid].pid, SIGKILL);
					table[nid].active = 0;
					table[nid].nid = 0;
					table[nid].pid = 0;
				}
			}
//			printf("Retval: %d; Data is available now: %c %c %d %d \n",retval,ch1,ch2,tv.tv_sec,tv.tv_usec);
		} else {
			printf("Retval: %d; No data within five seconds.\n",retval);
		}
	}
	for (i = 0; i < node_num; ++i)
		free(table[i].pipe_fd_p);

	free(table);
}

