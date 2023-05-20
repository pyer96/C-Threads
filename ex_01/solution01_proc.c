/*############################_Pier_Luigi_Manfrini_############################
 *
 *	This program attempts to create the maximum number of processes
 *	that we are able to create on the running machine
 *	
 *	(Running this code i managed to create up to 30000 processes
 *	before the load on the machine was such that user interfaces
 *	became laggish and non responsive, but fork() was still returning
 *	properly new children although really slowly)
 *	After the program has been stopped no system reboot was necessary
 *	since the resources are atomatically freed
 *	
 *	Usage:
 *		<./solution01_proc> 
 *	
 *	Note:
 *		-Ctrl-C to stop execution!
 *
 *	Compile:
 *		gcc solution01_proc.c -o solution01_proc -Wall -Werror 
 *
 *#############################################################################
 */

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

bool run = true;
void handler(int sig) { run = false;
 signal(SIGINT, SIG_DFL);
}

int main(int argc, char **argv) {
  signal(SIGINT, handler);
  int counter = 0;
  pid_t pid;
  while ((pid = fork()) != -1 && run) {
    if (pid == 0) {
      while (run)
        ;
      exit(EXIT_SUCCESS);
      break;
    } else {
      dprintf(2, "Number of Processes Created:%d\n", counter++);
    }
  }
	while(wait(NULL)>0)
		;
  return 0;
}
