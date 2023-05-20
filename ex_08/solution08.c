/*################################_Pier_Luigi_Manfrini_####################################
 *
 *	This program check whether a given number N is prime or not. To perform the
 *	evaluation, P threads are created each in charge of trying to find eventual
 *	divisor of N in a smaller range assigned to it.
 *	N and P are provided by the user through CLI.
 *
 *	Usage:
 *		<./solution08> <N> <P>
 *
 *	Compile:
 *		gcc solution08.c -o solution08 -Wall -Werror -pthread -lm -fsanitize=leak
 *
 * ########################################################################################
 */
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

bool not_prime = false;

typedef struct {
  int number;
  int lower_bound;
  int upper_bound;
} thread_arg;

void *thr_job(void *arg);

int main(int argc, char **argv) {
  if (argc != 3) {
    dprintf(2, "Wrong Usage: <./solution08> <N> <P> \n");
    exit(EXIT_FAILURE);
  }
  int P;
  unsigned long long N;
  N = (unsigned long long)strtoull(argv[1], NULL, 10);
  P = atoi(argv[2]);
  if (N <= 0 || P <= 0) {
    dprintf(2, "N and P must both be positive integers!\n");
    exit(EXIT_FAILURE);
  }
  if (N <= 2) {
    dprintf(1,
            "Why are you even trying to evaluate this number?\nNobody really "
            "agrees whether %llu is a true prime or not!\n",
            N);
    exit(EXIT_SUCCESS);
  }

  clock_t begin = clock();

  /* Threads initialization */
  pthread_t *thread_ID = (pthread_t *)malloc(P * sizeof(pthread_t));
  pthread_attr_t attribute;
  pthread_attr_init(&attribute);
  pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_JOINABLE);
  thread_arg *args = (thread_arg *)malloc(P * sizeof(thread_arg));

  /* Threads Creation */
  for (int i = 0; i < P; i++) {
    /* Setting the ranges that each thread is in charge of */
    args[i].number = N;
    args[i].lower_bound = floor(floor(N / 2) / P) * i + 2;
    if (i == P - 1)
      args[i].upper_bound = floor(N / 2);
    else
      args[i].upper_bound = args[i].lower_bound + (floor(floor(N / 2) / P) - 1);

    /* Thread Create */
    pthread_create(&thread_ID[i], &attribute, thr_job, (void *)&args[i]);
  }

  for (int i = 0; i < P; i++) {
    if (not_prime) {
      clock_t end = clock();
      dprintf(1, "%llu is NOT PRIME!\t\t(took %7.6f seconds)\n", N,
              (double)(end - begin) / CLOCKS_PER_SEC);
      free(args);
      free(thread_ID);
      return 0;
    }
    pthread_join(thread_ID[i], NULL);
  }
 clock_t end = clock();

  if (!not_prime)
    dprintf(1, "%llu is PRIME!\t\t(took %7.6f seconds)\n", N,(double)(end-begin)/CLOCKS_PER_SEC);
  else
    dprintf(1, "%llu is NOT PRIME!\t\t(took %7.6f seconds)\n", N,(double)(end-begin)/CLOCKS_PER_SEC);
  free(args);
  free(thread_ID);
  return 0;
}

void *thr_job(void *arg) {
  thread_arg *input = arg;
  for (int i = input->lower_bound; i <= input->upper_bound; i++) {
    if (not_prime)
      break;
    if ((input->number % i) == 0)
      not_prime = true;
  }
  pthread_exit(EXIT_SUCCESS);
}
