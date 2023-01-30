#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>

#define NUM_THREADS 10
#define NUM_ITER 10000

int count = 0;
pthread_mutex_t mutex;

void *counter(void *data)
{
    int thread_id = *((int *)data);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < NUM_ITER; i++)
    {
        count++;
    }
    pthread_mutex_unlock(&mutex);
    return 0;
}

int main(int argc, char *argv[])
{

    pthread_t mythreads[NUM_THREADS];
    int mythread_id[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++)
    {
        mythread_id[i] = i;
        pthread_create(&mythreads[i], NULL, counter, (void *)&mythread_id[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(mythreads[i], NULL);
    }

    printf("count = %d\n", count);

    return 0;
}