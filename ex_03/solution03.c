/*############################_Pier_Luigi_Manfrini_###################################
 *
 * 	This Program creates N threads and has two different counters
 * 	initialized to zero.
 * 	At execution, each thread, draws a random number and, depending if
 * 	the drawed number is even or odd, it increments by 1 the corresponding
 * 	counter.
 * 	N is provided by the user through CLI.
 *
 * 	Usage:
 * 		<./solution03> <N>
 *	
 *	Notes:
 *		-Even if only one mutex could have been sufficient, in order
 *		to speed up thing, two mutexes have been involved so that
 *		if one thread draws an even number and another thread
 *		draws an odd number they avoid waiting on the same mutex.
 *
 * 	Compile:
 * 		gcc solution03.c -o solution03 -Wall -Werror -pthread -fsanitize=leak
 *
 *####################################################################################
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RANGE                                                                  \
  100 // Useful to define smaller range instead of the rand() default one ( [0
      // RAND_MAX] )

static unsigned int even_counter = 0;
static unsigned int odd_counter = 0;

pthread_mutex_t mutex_even;
pthread_mutex_t mutex_odd;

void *thread_work(void *arg) {
  unsigned int rnd_numb = (unsigned int)rand() % RANGE;
  if (!(rnd_numb % 2)) { // even number
    dprintf(1, "Drawn %d \t>>\tAcquiring Lock for the even counter\n", rnd_numb);
    pthread_mutex_lock(&mutex_even);
    even_counter++;
    pthread_mutex_unlock(&mutex_even);
  } else {
    dprintf(1, "Drawn %d \t>>\tAcquiring Lock for the odd counter\n", rnd_numb);
    pthread_mutex_lock(&mutex_odd);
    odd_counter++;
    pthread_mutex_unlock(&mutex_odd);
  }
  pthread_exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
  pthread_mutex_init(&mutex_even, NULL);
  pthread_mutex_init(&mutex_odd, NULL);
  srand((unsigned int)time(NULL));
  if (argc != 2) {
    dprintf(2, "Wrong Usage: <./solution03> <N>\n");
    exit(EXIT_FAILURE);
  }
  int N = atoi(argv[1]);
  if (N <= 0) {
    dprintf(2, "N must be a positive integer!\n");
    exit(EXIT_FAILURE);
  }
  dprintf(1,
          "Actual value of counters:\neven_counter :%d\nodd_counter "
          ":%d\nCreating %d threads...\nCreating 2 mutexes...\n",
          even_counter, odd_counter, N);
  pthread_t *threads = (pthread_t *)malloc(N * sizeof(pthread_t));
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  for (int i = 0; i < N; i++) {
    pthread_create(&threads[i], &attr, thread_work, NULL);
  }
  int j = 0;
  while (pthread_join(threads[j], NULL) == 0){
   j++;
   if (j==N)break;
  }
  free(threads);
  dprintf(1, "Final value of counters:\neven_counter :%d\nodd_counter :%d\n",
          even_counter, odd_counter);
  return 0;
}
