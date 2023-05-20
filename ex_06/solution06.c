/*##############################_Pier_Luigi_Manfrini_#################################
 *	
 *	This program sets up a basic Producer-Consumer situation.
 *	The main thread generates N produrers and N consumers where N is provided
 *	through CLI by the user.
 *	There are globally visible 10 items represented by an integer array
 *	stored in the process' heap. Each integer in the array indicates
 *	the quantity in stock for each of the items.
 *	
 *	Usage:
 *		<./solution06> <N>
 *
 *	Compile:
 *		gcc solution06.c -o solution06 -Wall -Werror -pthread -fsanitize=leak
 *
 *	Note:
 *		-Program comes with a defined DEBUG macro variable that is,
 *		by default, set to one. This allows to have a look on what is going
 *		on: how many stocks are present and how many stocks are consumed.
 *		However, with the DEBUG mode turned on, the program is slowed down
 *		a bit by the introduction of a new mutex that locks the whole
 *		array of items.
 *		-To avoid confusion in the STDOUT only one Producer (the first) is 
 *		in charge of reporting the remaining stocks, so the report printed
 *		on STDOUT is never actually up-to-date but serves to provide
 *		a general view of the situation. Instead, item consumption is 
 *		reported by every consumer right after having consumed the item.
 *		-Since the ITEMS are formatted in columns, it is suggested to run 
 *		the program in a fully expanded terminal window.
 *		-Since making the Producer sleep for 2 second for every item was 
 *		way too slow, every producer waits for 2 seconds for every 10 items
 *		update, which is more reasonable and pleasing for the user.
 *		-To stop the whole Production-Consumption process the user can 
 *		use the interrupt SIGINT (Ctrl-C) which is captured by the main 
 *		thread and causes pthread_cancellation.
 *
 * ###################################################################################
 */

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define RED "\033[1;31m"
#define red "\033[0;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define yellow "\033[0;33m"
#define CYAN "\033[1;36m"
#define cyan "\033[0;36m"
#define MAGENTA "\033[1;35m"
#define magenta "\033[0;35m"
#define RESET "\033[0m"

#define DEBUG 1 // or 0

int ITEMS[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

pthread_mutex_t dbg_all;
pthread_mutex_t item[10];
pthread_cond_t enough[10];

int N;
pthread_t *thread_IDs_prod;
pthread_t *thread_IDs_cons;

/* PRODUCER ROUTINE */
void *producer(void *arg) {
  int index = *(int *)arg;
  int update_vector[10];
  while (1) {
    /* Update Pre-Computing */
    for (int i = 0; i < 10; i++) {
      update_vector[i] =
          1 +
          rand() % 10; // Producer produces random quantity [1 10] for each item
    }
      sleep(2);
    /* Update ITEMS */
    for (int i = 0; i < 10; i++) {
      pthread_mutex_lock(&item[i]);
      ITEMS[i] += update_vector[i];
      pthread_cond_signal(&enough[i]);
      pthread_mutex_unlock(&item[i]);
    }
    if (DEBUG && index == 0) { // If in DEBUG mode, we make one Producer (the
                               // first one) report the Stocks' situation
      pthread_mutex_lock(&dbg_all);
      dprintf(1,MAGENTA"produced_STOCKS>\t"RESET);
      for (int i = 0; i < 10; i++) {
        dprintf(
            1, RED "ITEM[" CYAN "%d" RED "]" RESET ": " GREEN "%d" RESET "\t%c",
            i, ITEMS[i], i == 9 ? '\n' : ' ');
      }
      pthread_mutex_unlock(&dbg_all);
    }
  }
  pthread_exit(EXIT_SUCCESS);
}

/* CONSUMER ROUTINE */
void *consumer(void *arg) {
  while (1) {
    int random_item = rand() % 10; // Consumer select one random item to consume
    int random_quantity =
        1 + rand() % 100; // Consumer consumer random quantity [1 100] of 1 item
    pthread_mutex_lock(&item[random_item]);
    while (ITEMS[random_item] < random_quantity)
      pthread_cond_wait(&enough[random_item], &item[random_item]);
    ITEMS[random_item] -= random_quantity;
    
    if(DEBUG){
    pthread_mutex_lock(&dbg_all);
    dprintf(1,yellow"consumed_STOCKS>"RED"%sITEM[" CYAN "%d" RED "]" RESET":"YELLOW" -%d"RESET"\n", random_item==0?"\t": random_item==1 ? "\t\t\t " : random_item==2?"\t\t\t\t\t ": random_item==3? "\t\t\t\t\t\t\t " : random_item==4? "\t\t\t\t\t\t\t\t\t " : random_item==5?"\t\t\t\t\t\t\t\t\t\t\t ":random_item==6?"\t\t\t\t\t\t\t\t\t\t\t\t\t ":random_item==7?"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t ":random_item==8?"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t ":"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t "  ,random_item,random_quantity );
    pthread_mutex_unlock(&dbg_all);
    } 
    
    pthread_mutex_unlock(&item[random_item]);
  }
  pthread_exit(EXIT_SUCCESS);
}

void handler(int signum) {
  for (int i = 0; i < N; i++) {
    pthread_cancel(thread_IDs_cons[i]);
    pthread_cancel(thread_IDs_prod[i]);
  }
  signal(SIGINT, SIG_DFL);
}

int main(int argc, char **argv) {
  if (argc != 2) {
    dprintf(2, "Wrong Usage: <./solution06> <N>\n");
    exit(EXIT_FAILURE);
  }
  N = atoi(argv[1]);
  if (N <= 0) {
    dprintf(2, "N must be a positive integer!\n");
    exit(EXIT_FAILURE);
  }
  srand((unsigned int)time(NULL));

  /* Capturing SIGINT in order to change its default behaviour */
  struct sigaction action = {0};
  action.sa_handler = handler;
  action.sa_flags = 0;
  sigaction(SIGINT, &action, NULL);

  /* Mutexes and Conditional variables initialization */
  for (int i = 0; i < 10; i++) {
    pthread_mutex_init(&item[1], NULL);
    pthread_cond_init(&enough[i], NULL);
  }

  thread_IDs_prod = (pthread_t *)malloc(N * sizeof(pthread_t));
  thread_IDs_cons = (pthread_t *)malloc(N * sizeof(pthread_t));
  pthread_attr_t attribute;
  pthread_attr_init(&attribute);
  pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_JOINABLE);
  dprintf(1,
          CYAN
          "Initializing %d Producer Threads and %d Consumer Threads..." RESET
          "\n",
          N, N);
  clock_t begin = clock();
  int * indexes = (int*)malloc(N*sizeof(int));
  /* Creation of N producers and N consumers */
  for (int i = 0; i < N; i++) {
	indexes[i] = i;	
    if (pthread_create(&thread_IDs_prod[i], &attribute, producer, (void *)&indexes[i]))
      perror("");
    if (pthread_create(&thread_IDs_cons[i], &attribute, consumer, NULL))
      perror("");
  }
  clock_t end = clock();
  dprintf(1, cyan "Initialization Completed!\t(took %7.6f seconds)" RESET "\n",
          (double)(end - begin) / CLOCKS_PER_SEC);
  for (int i = 0; i < N; i++) {
    pthread_join(thread_IDs_prod[i], NULL);
    pthread_join(thread_IDs_cons[i], NULL);
  }
  dprintf(1, CYAN "\b\bAll Producers and Consumers joined!" RESET "\n");
  free(thread_IDs_prod);
  free(thread_IDs_cons);
  free(indexes);
  return 0;
}
