/*######################################_Pier_Luigi_Manfrini_######################################
 *
 *	This is the exercise 09 reimplemented using the ThreadPool developed in exercise11!
 *	(N: program finds all prime numbers lower than N)
 *	(T: number of threads requested)
 *
 *	Usage:
 *		<./ex_09thrpool> <N> <T>
 *
 *	Notes:
 *		-DEBUG macro flag available in order to check ranges assigned to workers
 *
 *	Compile:
 *		gcc ex_09thrpool.c threadpool_handler.c -o ex_09thrpool -Wall -Werror -pthread -lm
 *
 * #################################################################################################
 */

#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "threadpool_handler.h"

#define RED "\033[1;31m"
#define RESET "\033[0m"

#define DEBUG 0 // 0 or 1

size_t size = 0;
int *prime_numbers = NULL;
size_t next_in = 0;

typedef struct {
  int lower_bound;
  int upper_bound;
} thr_arg;

pthread_mutex_t mutex;
pthread_cond_t cond;

void quicksort(int first, int last) {
  int i, j, pivot, temp;
  if (first < last) {
    pivot = first;
    i = first;
    j = last;

    while (i < j) {
      while (prime_numbers[i] <= prime_numbers[pivot] && i < last)
        i++;
      while (prime_numbers[j] > prime_numbers[pivot])
        j--;
      if (i < j) {
        temp = prime_numbers[i];
        prime_numbers[i] = prime_numbers[j];
        prime_numbers[j] = temp;
      }
    }

    temp = prime_numbers[pivot];
    prime_numbers[pivot] = prime_numbers[j];
    prime_numbers[j] = temp;
    quicksort(first, j - 1);
    quicksort(j + 1, last);
  }
}

void find_prime(void *arg) {
  thr_arg *input = arg;
  for (int i = input->lower_bound; i <= input->upper_bound; i++) {
    bool not_prime = false;
    if (i == 1)
      not_prime = true;
    for (int j = 2; j <= floor(i / 2); j++) {
      if (i % j == 0) {
        not_prime = true;
        break;
      }
    }
    if (!not_prime) {
      /* Mutex LOCK */
      pthread_mutex_lock(&mutex);
      /* reserve heap space the first time */
      if (prime_numbers == NULL) {
        size = 100;
        prime_numbers = (int *)malloc(size * sizeof(int));
      }
      /* Realloc if running out of space (75% filled up) */
      else if (next_in == size - floor(size * 75 / 100)) {
        size = (size_t)size * 1.5;
        prime_numbers = (int *)realloc(prime_numbers, size);
      }
      prime_numbers[next_in] = i;
      next_in++;
      pthread_mutex_unlock(&mutex);
      /* Mutex UNLOCK */
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    dprintf(2, "Wrong Usage: <./ex_09thrpool> <N> <T>\n");
    exit(EXIT_FAILURE);
  }
  int N, T;
  N = atoi(argv[1]);
  T = atoi(argv[2]);
  if (N <= 0 || T <= 0) {
    dprintf(2, "N and T must be positive integers!\n");
    exit(EXIT_FAILURE);
  } else if (N == 1) {
    dprintf(2, "Nobody knows...is 1 prime?\n");
    exit(EXIT_FAILURE);
  } else if (T > N / 2) {
    dprintf(2, "The number of threads (T) has to be lower than N/2!\n");
    exit(EXIT_SUCCESS);
  }
  clock_t begin = clock();
  /* Mutex Initialization */
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&cond, NULL);

  thr_arg *args = (thr_arg *)malloc(T * sizeof(thr_arg));
  /* Initializing T threads */
  threadpool_t *threadpool_HANDLER = threadpool_init(T);

  /* Threads Work assignment */
  for (int i = 0; i < T; i++) {
    args[i].lower_bound = (floor(N / T) * i) + 1;
    if (DEBUG)
      dprintf(1, "thr %d LOWER:%d\t", i, args[i].lower_bound);
    if (i == T - 1)
      args[i].upper_bound = N;
    else
      args[i].upper_bound = args[i].lower_bound + (floor(N / T) - 1);
    if (DEBUG)
      dprintf(1, "UPPER:%d\n", args[i].upper_bound);
   if(!threadpool_launch(threadpool_HANDLER, find_prime, (void *)&args[i]))
	   dprintf(2,"Error while submitting this function to the threadpool!\n");
  }
  /* Wait for the job to finish */
  threadpool_wait(threadpool_HANDLER);

  /* Sort prime numbers' vector */
  quicksort(0, next_in - 1);
  clock_t end = clock();

  /* Print all prime numbers found */
  dprintf(1, RED "Prime Numbers smaller than %d:" RESET "\n", N);
  for (int i = 0; i < next_in; i++) {
    dprintf(1, "%d%s", prime_numbers[i], ((i + 1) % 15) ? "\t" : "\n");
  }
  dprintf(1, "\n(took %7.6f seconds)\n",
          (double)(end - begin) / CLOCKS_PER_SEC);

  free(args);
  free(prime_numbers);
  
  threadpool_destroy(threadpool_HANDLER);
  return 0;
}
