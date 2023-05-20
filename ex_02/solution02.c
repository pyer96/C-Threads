/*##############################_Pier_Luigi_Manfrini_##################################
 *
 *	This program creates N threads; each one is going to sleep
 *	for a random time ranging between 0 and 10 seconds.
 *	The main thread will wait for all the N other threads and
 *	then return.
 *	N is given by th user through CLI.
 *
 *	Usage: <./solution02> <N>
 *
 *	Compile:
 *		gcc solution02.c -o solution02 -Wall -Werror -pthread -fsanitize=leak
 *
 * ####################################################################################
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void * thread_sleep(void* arg){
int num =*(int*)arg + 1;
unsigned int time = rand() % 11;
dprintf(1,"Hi from thread #%d, I'm going to sleep for %d seconds\n", num, time);
sleep(time);
pthread_exit(EXIT_SUCCESS);
}


int main(int argc, char** argv){
srand((unsigned int)time(NULL));
if(argc!=2){
  dprintf(2,"Wrong Usage: <./solution02> <N>\n");     
  exit(EXIT_FAILURE);
} 

int N = atoi(argv[1]);
int *num = (int*)malloc(N*sizeof(int));
pthread_t * ids = (pthread_t *)malloc(N*sizeof(pthread_t));
pthread_attr_t attr;
pthread_attr_init(&attr);
pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); 
for (int i = 0 ; i < N; i++){
	num[i] = i; 
	if(pthread_create(&ids[i], &attr, thread_sleep, &num[i])!= 0){
		perror("");
		exit(EXIT_FAILURE);
	}
}
int j  = 0;
while( j < N && pthread_join(ids[j], NULL)==0){
dprintf(1, "Thread #%d finished waiting!\n",j+1);
j++;
}
free(ids);
free(num);

return 0;
}
