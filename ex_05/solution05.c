/*#########################_Pier_Luigi_Manfrini_############################
 *
 *	This program creates two children processes. Each creates its own
 *	additional (apart from their main thread) thread.
 *
 *	The purpose of this code is to test intercommunication between
 *	processes and threads.
 *	Goal: Send a single byte across threads (from parent process to all
 *	the child processes' threads)
 *
 *	Usage:
 *		<./solution05>
 *
 *
 *	Compile:
 *		gcc solution05.c -o solution05 -Wall -Werror -pthread
 *			-fsanitize=leak
 *
 * #########################################################################
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define READ 0
#define WRITE 1
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define cyan "\033[0;36m"
#define reset "\033[0m"

struct thread_args {
  char letter;
  int order;
};

void *job(void *arg) {
  struct thread_args args = *(struct thread_args *)arg;
  dprintf(1, "Letter (byte) received by"RED" secondary thread"reset" in %s child:\t"GREEN" %c"reset"\n",
          args.order == 0 ? "first" : "second", args.letter);
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  if (argc != 1) {
    dprintf(2, "No arguments needed for this program!\n");
    exit(EXIT_FAILURE);
  }
  pid_t *children_id = (pid_t *)malloc(2 * sizeof(pid_t));
  int father2child[2][2];
  pipe(father2child[0]);
  pipe(father2child[1]);
  for (int i = 0; i < 2; i++) {
    if ((children_id[i] = fork()) == -1) { // Failure
      perror("");
      exit(EXIT_FAILURE);
    } else if (children_id[i] == 0) { // Child
      close(father2child[i][WRITE]);
      pthread_t thread;
      pthread_attr_t attribute;
      pthread_attr_init(&attribute);
      pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_JOINABLE);
      char letter;
      read(father2child[i][READ], &letter, 1);
      dprintf(1, "Letter (byte) received by"cyan" main thread"reset" of %s child:\t\t"GREEN" %c"reset"\n",
              i == 0 ? "first" : "second", letter);
      struct thread_args args = {letter, i};
      pthread_create(&thread, &attribute, job, (void *)&args);

      pthread_join(thread, NULL);
      exit(EXIT_SUCCESS);
      break;
    } else { // Father
      close(father2child[i][READ]);
    }
  }

  /* After children generation the father send to them the single byte with the
   * letter P as it is the initial letter of the programmer's name */
  char letter = 'P';
  dprintf(1,"Letter (byte) Sent by main process:\t"GREEN" %c"reset"\n",letter);
  for (int i = 0; i < 2; i++) {
    write(father2child[i][WRITE], &letter, 1);
  }

  for (int i = 0; i < 2; i++) {
    waitpid(children_id[i], 0, 0);
  }
  free(children_id);
  return 0;
}
