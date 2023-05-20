/*############################_Pier_Luigi_Manfrini_###############################
 *
 *	This is the functions' implementation of a threadpool handler.
 *	The threadpool API's are listed in the corresponding header file
 *	"threadpool_handler.h".
 *	For this project, since I did not know exactly where to start and
 *	how to structure the library, I resorted to John Nachtimwald's online
 *	threadpool's explanation.
 *	
 *	Usage:
 *		compile this along with the program in which threadpool is used
 *
 * ###############################################################################
 */

#include <stdio.h>
#include <stdlib.h>

#include "threadpool_handler.h"

static threadpool_work_t *create_work(thread_function_t func, void *arg) {
  if (func == NULL)
    return NULL;
  /* Append a new work struct to the list */ 
  threadpool_work_t *work = malloc(sizeof(threadpool_work_t));
  work->argument = arg;
  work->function = func;
  work->next = NULL;
  return work;
}

static threadpool_work_t *get_work(threadpool_t *th) {
  if (th == NULL)
    return NULL;
  threadpool_work_t *work = th->first_work;
  if (work == NULL)
    return NULL;
  /* If we pulled out the last pending work */
  if (work->next == NULL) {
    th->first_work = NULL;
    th->last_work = NULL;
  } else { /* If we pulled out not the last pending work */
    th->first_work = work->next;
  }
  return work;
}

static void destroy_work(threadpool_work_t *work) {
  if (work == NULL)
    return;
  free(work);
}

static void *threadpool_worker(void *arg) {
  threadpool_t *th = arg;
  threadpool_work_t *work;
  /* The worker is always active waiting for more work to process */
  while (1) {
    pthread_mutex_lock(&(th->mutex));
    /* If there is no work, but the threadpool is running the thread waits */
    while (th->first_work == NULL && !th->stop)
      pthread_cond_wait(&(th->available_work), &(th->mutex));
    if (th->stop)
      break;
    /* The thread gets the first work in queue to execute it */
    work = get_work(th);
    th->num_working++;
    pthread_mutex_unlock(&(th->mutex));

    if (work != NULL) {
      work->function(work->argument);
      /* Once finished its work the thread frees the memory for its struct and
       * decrements num_working, because there is one less worker active */
      destroy_work(work);
    }

    pthread_mutex_lock(&(th->mutex));
    th->num_working--;
    if (!th->stop && th->num_working == 0 && th->first_work == NULL)
      pthread_cond_signal(&(th->nobody_working));
    pthread_mutex_unlock(&(th->mutex));
  }
  /* If stop == true thread will erase itself from the count of available
   * threads and signal that ther is one less working */
  th->num_threads--;
  pthread_cond_signal(&(th->nobody_working));
  pthread_mutex_unlock(&(th->mutex));
  return NULL;
}

threadpool_t *threadpool_init(size_t num) {
  dprintf(1, green "Initializing the Thread Pool..." reset "\n");
  pthread_t thread;
  size_t i;
  /* If zero is passed the default number of created threads is 6 as the number
   * of core of my machine (excluding thread's virtualization) */
  if (num == 0)
    num = 6;
  /*
   * Initialize the core threadpool struct, the mutex, and the two conditionals.
   * No need for acquiring the mutex's lock since no threads are created yet
   */
  threadpool_t *th = calloc(1, sizeof(threadpool_t));
  th->num_threads = num;
  pthread_mutex_init(&(th->mutex), NULL);
  pthread_cond_init(&(th->available_work), NULL);
  pthread_cond_init(&(th->nobody_working), NULL);
  th->first_work = NULL;
  th->last_work = NULL;
  /* Threads Creation with pthread_create() */
  for (i = 0; i < num; i++) {
    pthread_create(&thread, NULL, threadpool_worker, (void *)th);
    pthread_detach(thread);
  }
  dprintf(1,GREEN"Pool of %zu threads ready!"reset"\n\n",num);
  return th;
}

bool threadpool_launch(threadpool_t *th, thread_function_t func, void *arg) {
  if (th == NULL)
    return false;
  threadpool_work_t *work = create_work(func, arg);
  if (work == NULL)
    return false;
  /* Add work to the list and signal it */
  pthread_mutex_lock(&(th->mutex));
  if (th->first_work == NULL) {
    th->first_work = work;
    th->last_work = th->first_work;
  } else {
    th->last_work->next = work;
    th->last_work = work;
  }
  pthread_cond_broadcast(&(th->available_work));
  pthread_mutex_unlock(&(th->mutex));
  return true;
}

void threadpool_wait(threadpool_t *th) {
  if (th == NULL)
    return;
  /* Waits, if necessary, on the conditional variable that gets signalled when
   * no thread is working anymore */
  pthread_mutex_lock(&(th->mutex));
  while (1) {
    if ((!th->stop && th->first_work!=NULL /* th->num_working != 0 */) ||
        (th->stop && th->num_threads != 0)) {
      pthread_cond_wait(&(th->nobody_working), &(th->mutex));
    } else {
      break;
    }
  }
  pthread_mutex_unlock(&(th->mutex));
}

void threadpool_destroy(threadpool_t *th) {
  if (th == NULL)
    return;
  threadpool_work_t *work;
  threadpool_work_t *tmp;
  pthread_mutex_lock(&(th->mutex));
  /* We free all possible works still pending */
  work = th->first_work;
  while (work != NULL) {
    tmp = work->next;
    destroy_work(work);
    work = tmp;
  }
  th->stop = true;
  /* To avoid threads stucked in this conditional */
  pthread_cond_broadcast(&(th->available_work));
  pthread_mutex_unlock(&(th->mutex));

  dprintf(1,"\n"green"Stopping all threads..."reset"\n");
  /* Ensure no thread is running anymore */
  threadpool_wait(th); 

  /* Release Mutex and Conditionals resources */
  pthread_mutex_destroy(&(th->mutex));
  pthread_cond_destroy(&(th->available_work));
  pthread_cond_destroy(&(th->nobody_working));
  free(th);
  dprintf(1,GREEN"Thread Pool freed!"reset"\n");
}
