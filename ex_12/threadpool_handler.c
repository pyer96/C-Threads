/*############################_Pier_Luigi_Manfrini_###############################
 *
 *	Improvement of the Threadpool Handler developed for exercise 11.
 *	Scheduler (random selection) has been added along with the
 *      implementation of a ready and a running queue for the pooled threads.
 *
 *      Notes: 
 *      	- Max contemporary workers = MACHINE_CORES (macro defined as 6)
 *      	- Random selection of the Scheduler among the threads
 *      	  in the ready queue
 *
 *	Usage:
 *		compile this along with the program in which threadpool is used
 *
 * ###############################################################################
 */

#include <stdio.h>
#include <stdlib.h>

#include "threadpool_handler.h"

/* ---------------------CREATE WORK------------------ */
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

/* ----------------------GET WORK---------------- */
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

/* ----------------------DESTROY WORK------------------ */
static void destroy_work(threadpool_work_t *work) {
  if (work == NULL)
    return;
  free(work);
}

/* ---------------------CREATE ID--------------------- */
static thread_id_t *create_id(pthread_t id) {
  thread_id_t *instance = (thread_id_t *)malloc(sizeof(thread_id_t));
  instance->id = id;
  instance->next = NULL;
  return instance;
}

/* --------------------COUNT_QUEUE-------------------- */
size_t count_queue(thread_id_t *root) {
  thread_id_t *tmp = root;
  if (tmp == NULL)
    return 0;
  else {
    size_t cnt = 0;
    for (; tmp != NULL; tmp = tmp->next) {
      cnt++;
    }
    return cnt;
  }
}

/* ------------PULL_READY_PUSH_RUNNING------------ */
void pull_ready_push_running(threadpool_t *th) {
  /* During the execution of this function mutex is already locked by the caller
   */
  size_t random_thread_in = rand() % (count_queue(th->ready_queue_first));
  thread_id_t *instance = th->ready_queue_first;
  while (random_thread_in > 0) {
    instance = instance->next;
    random_thread_in--;
  }

  /* It is not a double linked list so we need to find the
   * instance prior to tmp */
  if (instance == th->ready_queue_first)
    th->ready_queue_first = instance->next;
  else if (instance == th->ready_queue_last) {
    thread_id_t *tmp;
    for (tmp = th->ready_queue_first; tmp->next != instance; tmp = tmp->next) {
    }
    tmp->next = NULL;
    th->ready_queue_last = tmp;
  } else {
    thread_id_t *tmp;
    for (tmp = th->ready_queue_first; tmp->next != instance; tmp = tmp->next) {
    }
    tmp->next = instance->next;
  }

  /* Append instance to running queue */
  instance->next = NULL;
  if (th->running_queue_first == NULL) {
    th->running_queue_first = instance;
    th->running_queue_last = th->running_queue_first;
  } else {
    th->running_queue_last->next = instance;
    th->running_queue_last = instance;
  }
}

/* ---------------PULL_RUNNING_PUSH_READY ---------------- */
void pull_running_push_ready(threadpool_t *th, pthread_t id) {
  /* During the execution of this function mutex is already locked by the caller
   */
  thread_id_t *tmp = th->running_queue_first;
  thread_id_t *prev = NULL;
  if (tmp == NULL) {
   // dprintf(2, "error within running  queue\n");
    return;
  }
  for (; tmp->next != NULL; tmp = tmp->next) {
    if (pthread_equal(tmp->id, pthread_self()) != 0)
      break;
    prev = tmp;
  }

  /* if the first in queue */
  if (prev == NULL)
    th->running_queue_first = tmp->next;
  /* if last in queue */
  else if (tmp == th->running_queue_last) {
    prev->next = NULL;
    th->running_queue_last = prev;
  }
  /* if in the middle */
  else {
    prev->next = tmp->next;
  }

  /* append back the instance to the ready queue */
  tmp->next = NULL;
  if (th->ready_queue_first == NULL) {
    th->ready_queue_first = tmp;
    th->ready_queue_last = th->ready_queue_first;
  } else {
    th->ready_queue_last->next = tmp;
    th->ready_queue_last = tmp;
  }
}

/* -----------------SCHEDULER--------------- */
void *scheduler(void *arg) {
  threadpool_t *th = arg;
  if (th == NULL) {
    dprintf(2, " impossible to initiate scheduler \n");
    return NULL;
  }
  while (1) {
    pthread_mutex_lock(&(th->mutex));
    /* I bound the max num of contemporary threads to the number of core of my
     * machine */
    while(!th->stop && (th->num_working >= MACHINE_CORES || count_queue(th->running_queue_first) >= MACHINE_CORES)) {
	  pthread_cond_wait(&(th->sched_max), &(th->mutex));  
    }

    if (th->stop) {
     // dprintf(2, "sched stopped\n");
      break;
    }
    /* Switch thread from ready to running queue */
    if (count_queue(th->ready_queue_first) != 0 && th->first_work!= NULL && th->num_working < MACHINE_CORES) {
      pull_ready_push_running(th);
      /*dprintf(2, "SCHED: RUNNING QUEUE LENGHT: %zu num_working: %zu\n",
              count_queue(th->running_queue_first), th->num_working);
      dprintf(2, "       READY QUEUE LENGHT: %zu num_working: %zu\n",
              count_queue(th->ready_queue_first), th->num_working);*/
      pthread_cond_signal(
          &(th->scheduled)); ////// CHECK IF REALLY NEEDED ????
    }
    pthread_mutex_unlock(&(th->mutex));
  }
  pthread_mutex_unlock(&(th->mutex));
  return NULL;
}

/* ---------------CAN'T RUN----------------- */
bool cant_run(threadpool_t *th, pthread_t id) {
  /* No need for mutex as it is already locked when this function is called */
  thread_id_t *tmp = th->ready_queue_first;
  if (tmp == NULL)
    return true;
  // dprintf(2,"DBG\n");
  for (; tmp->next != NULL; tmp = tmp->next) {
    /* pthread_equal returns non zero if equal */
    if (pthread_equal(id, tmp->id) != 0) {
      return true;
    }
  }
  return false;
}

/* --------------------THREADPOOL WORKER----------------- */
static void *threadpool_worker(void *arg) {
  threadpool_t *th = arg;
  threadpool_work_t *work;
  /* The worker is always active waiting for more work to process */
  while (1) {
    pthread_mutex_lock(&(th->mutex));
    while (!th->stop && cant_run(th, pthread_self())) {
      pthread_cond_wait(&(th->scheduled),&(th->mutex));
      continue;
    }
    /* If there is no work, but the threadpool is running, or the thread is not
     * scheduled, the thread waits */
    while (th->first_work == NULL && !th->stop)
      pthread_cond_wait(&(th->available_work), &(th->mutex));
    if (th->stop)
      break;

    /* The thread gets the first work in queue to execute it */
    th->num_working++;
    work = get_work(th);
    pthread_mutex_unlock(&(th->mutex));

    if (work != NULL) {
      work->function(work->argument);
      /* Once finished its work the thread frees the memory for its struct and
       * decrements num_working, because there is one less worker active */
      destroy_work(work);
    }

    pthread_mutex_lock(&(th->mutex));
    pull_running_push_ready(th, pthread_self());
    th->num_working--;
    if (th->num_working < MACHINE_CORES)
      pthread_cond_signal(&(th->sched_max));
    if (!th->stop && th->num_working == 0)
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

/* ----------------------THREADPOOL INIT-------------------*/
threadpool_t *threadpool_init(size_t num) {
  dprintf(1, green "Initializing the Thread Pool..." reset "\n");
  srand((unsigned int)time(NULL));
  pthread_t thread;
  size_t i;
  /* If zero is passed the default number of created threads is 6 as the number
   * of core of my machine (excluding thread's virtualization) */
  if (num == 0)
    num = DEFAULT_POOL;
  /*
   * Initialize the core threadpool struct, the mutex, and the two conditionals.
   * No need for acquiring the mutex's lock since no threads are created yet
   */
  threadpool_t *th = calloc(1, sizeof(threadpool_t));
  th->num_threads = num;
  pthread_mutex_init(&(th->mutex), NULL);
  pthread_cond_init(&(th->available_work), NULL);
  pthread_cond_init(&(th->nobody_working), NULL);
  pthread_cond_init(&(th->sched_max), NULL);
  pthread_cond_init(&(th->scheduled),NULL);
  th->first_work = NULL;
  th->last_work = NULL;
  /* Scheduler Support */
  th->ready_queue_first = NULL;
  th->ready_queue_last = NULL;
  th->running_queue_first = NULL;
  th->running_queue_last = NULL;

  /* Threads Creation with pthread_create() */
  pthread_mutex_lock(
      &(th->mutex)); // to make sure one thread doesnt test if he is in the
                     // ready/running queue even before the latter being updated
                     // with his own thread id
  for (i = 0; i < num; i++) {
    pthread_create(&thread, NULL, threadpool_worker, (void *)th);
    pthread_detach(thread);
    thread_id_t *instance = create_id(thread);
    if (th->ready_queue_first == NULL) {
      th->ready_queue_first = instance;
      th->ready_queue_last = th->ready_queue_first;
    } else {
      th->ready_queue_last->next = instance;
      th->ready_queue_last = instance;
    }
  }
  pthread_mutex_unlock(&(th->mutex));
  /* Scheduler Creation */
  pthread_create(&thread, NULL, scheduler, (void *)th);
  pthread_detach(thread);
  dprintf(1, GREEN "Pool of %zu threads ready!" reset "\n\n", num);
  return th;
}

/* -------------
 * ---------THREADPOOL LAUNCH------------------- */
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

/* --------------------THREADPOOL WAIT--------------------- */
void threadpool_wait(threadpool_t *th) {
  if (th == NULL)
    return;
  /* Waits, if necessary, on the conditional variable that gets signalled when
   * no thread is working anymore */
  pthread_mutex_lock(&(th->mutex));
  while (1) {
	 pthread_cond_broadcast(&(th->scheduled));///////TEST
    if ((!th->stop && th->first_work!=NULL/*th->num_working != 0 && count_queue(th->running_queue_first)!= 0 */) ||
        (th->stop && th->num_threads != 0)) {
      pthread_cond_wait(&(th->nobody_working), &(th->mutex));
    } else {
      break;
    }
  }
  pthread_mutex_unlock(&(th->mutex));
}

/* ---------------------THREADPOOL DESTROY-------------------- */
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
  /* To avoid threads stucked in this conditionals */
  pthread_cond_broadcast(&(th->scheduled));
  pthread_cond_broadcast(&(th->sched_max)); ////////////////////////////
  pthread_cond_broadcast(&(th->available_work));
  pthread_mutex_unlock(&(th->mutex));

  dprintf(1, "\n" green "Stopping all threads..." reset "\n");
  /* Ensure no thread is running anymore */
  threadpool_wait(th);
  /* Release Mutex and Conditionals resources */
  pthread_mutex_destroy(&(th->mutex));
  pthread_cond_destroy(&(th->available_work));
  pthread_cond_destroy(&(th->nobody_working));
  pthread_cond_destroy(&(th->sched_max));
  //dprintf(2, "DEEEEEBG\n");
  pthread_cond_destroy(&(th->scheduled));
 
  /* Free the running/ready queues */
	thread_id_t * id;
	thread_id_t * temp;
	// free READY queue
	id=th->ready_queue_first;
	while(id!=NULL){
	temp = id->next;
	free(id);
	id=temp;
	}	
	// free RUNNING queue
	id=th->running_queue_first;
	while(id!=NULL){
	temp = id->next;
	free(id);
	id=temp;
	}	
		
  free(th);
  dprintf(1, GREEN "Thread Pool freed!" reset "\n");
}
