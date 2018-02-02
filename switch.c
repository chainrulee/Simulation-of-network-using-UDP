#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "structure.h"
// a simple pthread example 
// compile with -lpthreads
static volatile int linkk;
// create the function to be executed as a thread
void *thread(void *ptr)
{
    int type = (int) ptr;
	while (1) {     
        read(STDIN_FILENO, &linkk, sizeof(int));
		printf("children's pthread, got number: %d \n", linkk);
     }

    return  ptr;
}

int main(int argc, char **argv)
{
    // create the thread objs
    pthread_t thread1, thread2;
    int thr = 1;
    int thr2 = 2;
    // start the threads
    pthread_create(&thread1, NULL, *thread, (void *) thr);
    //pthread_create(&thread2, NULL, *thread, (void *) thr2);
    // wait for threads to finish

    //pthread_join(thread1,NULL);
	int i;
	for (i = 0; i < argc; ++i) {
		printf("argv[%d] is %s \n", i, argv[i]);
	}
    while (1) {
        printf(" Main switch funciton \n");
        sleep(5);
     }
	//pthread_join(thread2,NULL);
    return 0;
}
