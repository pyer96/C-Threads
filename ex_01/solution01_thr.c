/*############################_Pier_Luigi_Manfrini_############################
 *
 *	This program attempts to create the maximum number of threads
 *	that we are able to create on the running machine
 *	Running this test I was able to create about 400000 threads before the
 *	system became unmanageable.
 *	Creation times, compared to the creation time for processes, are
 *	on average 20-30x times faster.
 *	No reboot needed if the threads are correctly stopped by a SIGINT and
 *	their allocated resources (mainly their stack and register)
 *	
 *	Usage:
 *		<./solution01_thr>
 *	
 *	Note: 
 *		-Ctrl-C to stop execution!
 *
 *	Compile:
 *		gcc solution01_thr.c -o solution01_thr -pthread -Wall -Werror
 *
 * ############################################################################
 */

#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool run = true;
void handler(int sig) {
  run = false;
  signal(sig, SIG_DFL);
}

void *thread_job(void *arg) {
  while (run)
    ;
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  signal(SIGINT, handler);
  int counter = 0;
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  while (run && pthread_create(&thread, &attr, thread_job, NULL)==0) {
    dprintf(2, "Number of Threads Created:%d\n", counter++);
  }
 
  return 0;
}
