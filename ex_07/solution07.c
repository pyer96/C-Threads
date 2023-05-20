/*################################_Pier_Luigi_Manfrini_###################################
 *
 *	This program evaluates the following math series:
 *
 *             		          ___N
 *                               \      (x^i)
 *                                .     _____
 *                               /__i=0 (y^i)
 *
 * 	The program is multithread, 2*N threads are created. Each thread computes 
 * 	a single x^i OR y^i instance.
 * 	The main thread, then, computes the divisions and the final summation.
 *
 * 	Usage:
 * 		<./solution07> <x> <y> <N>
 *
 * 	Compile:
 * 		gcc solution07.c -o solution07 -Wall -Werror -pthread -fsanitize=leak
 *
 * #######################################################################################
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  double base;
  int exponent;
  double result;
} thread_arguments;

void *num(void *);
void *den(void *);
double exponential(double, int);

int main(int argc, char **argv) {
  if (argc != 4) {
    dprintf(2, "Wrong Usage: <./solution07? <x> <y> <N>\n");
    exit(EXIT_FAILURE);
  }

  int N;
  double x, y;
  x = (double)strtod(argv[1], NULL);
  y = (double)strtod(argv[2], NULL);
  N = atoi(argv[3]);
  if (N <= 0) {
    dprintf(2, "N must be a positive integer!\n");
    exit(EXIT_FAILURE);
  }

  /* Threads Initialization */
  pthread_t *numerator = (pthread_t *)malloc(N * sizeof(pthread_t));
  pthread_t *denominator = (pthread_t *)malloc(N * sizeof(pthread_t));
  pthread_attr_t attribute;
  pthread_attr_init(&attribute);
  pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_JOINABLE);

  thread_arguments *args_num =
      (thread_arguments *)malloc(N * sizeof(thread_arguments));
  thread_arguments *args_den =
      (thread_arguments *)malloc(N * sizeof(thread_arguments));

  for (int i = 0; i < N; i++) {
    args_num[i].base = x;
    args_den[i].base = y;
    args_num[i].exponent = i + 1;
    args_den[i].exponent = i + 1;
    pthread_create(&numerator[i], &attribute, num, (void *)&args_num[i]);
    pthread_create(&denominator[i], &attribute, den, (void *)&args_den[i]);
  }

  for (int i = 0; i < N; i++) {
    pthread_join(numerator[i], NULL);
    pthread_join(denominator[i], NULL);
  }
  
  /* Main Thread Computes the final result */
double result = 0;
  for (int i = 0; i<N;i++){
	result += args_num[i].result/args_den[i].result;
  }
dprintf(1,"The Result of the Summation is:\t%f\n",result);
  free(numerator);
  free(denominator);
  free(args_num); 
  free(args_den);
  return 0;
}

void *num(void *arg) {
  thread_arguments *input = (thread_arguments *)arg;
  input->result = exponential(input->base, input->exponent);
  pthread_exit(EXIT_SUCCESS);
}

void *den(void *arg) {
  thread_arguments *input = (thread_arguments *)arg;
  input->result = exponential(input->base, input->exponent);
  pthread_exit(EXIT_SUCCESS);
}

double exponential(double x, int i) {
  /* Evaluates x^i */
  if (x == 0)
    return 0;
  double res = 1;
  for (int j = 1; j <= i; j++) {
    res = (double)(res * x);
  }
  return res;
}
