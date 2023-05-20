/*####################_Pier_Luigi_Manfrini_########################
 *
 *	Threadpool Handler API.
 *
 * ################################################################
 */

#ifndef __THR_POOL_HANDLER_H__
#define __THR_POOL_HANDLER_H__

#define RED "\033[1;31m"
#define red "\033[0;31m"
#define GREEN "\033[1;32m"
#define green "\033[0;32m"
#define CYAN "\033[1;36m"
#define cyan "\033[0;36m"
#define reset "\033[0m"

#define MACHINE_CORES 6
#define DEFAULT_POOL 4

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

/* Thread function typedef */
typedef void (*thread_function_t)(void *);


struct thread_id;
typedef struct thread_id{
pthread_t id;
struct thread_id * next;
}thread_id_t;


/* This struct is the building block for the list of all pending works */
struct threadpool_work;
typedef struct threadpool_work {
  thread_function_t function;
  void *argument;
  struct threadpool_work *next;
} threadpool_work_t;

/* This is the main struct of the threadpool */
struct threadpool;
typedef struct threadpool {
  size_t num_working;
  size_t num_threads;
  threadpool_work_t *first_work;
  threadpool_work_t *last_work;
  thread_id_t * ready_queue_first; 
  thread_id_t * ready_queue_last; 
  thread_id_t * running_queue_first;
  thread_id_t * running_queue_last;
  pthread_mutex_t mutex;
  pthread_cond_t available_work;
  pthread_cond_t nobody_working;
  pthread_cond_t sched_max;
  pthread_cond_t scheduled;
  bool stop;
} threadpool_t;

threadpool_t *threadpool_init(size_t);
bool threadpool_launch(threadpool_t *, thread_function_t, void *);
void threadpool_wait(threadpool_t *);
void threadpool_destroy(threadpool_t *);

#endif
