#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>
#include "zemaphore.h"

#define NUM_THREADS 3
#define NUM_ITER 10

zem_t zem_t_threads[NUM_THREADS]; // one zemaphore per thread

void *justprint(void *data)
{
  int thread_id = *((int *)data);

  for (int i = 0; i < NUM_ITER; i++)
  {
    zem_down(&zem_t_threads[thread_id]); // wait for my turn
    printf("This is thread %d\n", thread_id);
    int next_thread = (thread_id + 1) % NUM_THREADS;
    zem_up(&zem_t_threads[next_thread]); // signal next thread
  }
  return 0;
}

int main(int argc, char *argv[])
{

  pthread_t mythreads[NUM_THREADS];
  int mythread_id[NUM_THREADS];

  for (int i = 0; i < NUM_THREADS; i++)
  {
    mythread_id[i] = i;
    zem_init(&zem_t_threads[i], i == 0 ? 1 : 0); // initialize zemaphores. thread 0 starts
    pthread_create(&mythreads[i], NULL, justprint, (void *)&mythread_id[i]);
  }

  for (int i = 0; i < NUM_THREADS; i++)
  {
    pthread_join(mythreads[i], NULL);
  }

  return 0;
}
