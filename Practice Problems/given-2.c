#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>

int total_threads;
volatile int count = 0;

pthread_mutex_t mutex;
pthread_cond_t *cond = NULL;

void *print(void *data)
{
    int thread_id = *((int *)data);

    while (1)
    {
        pthread_mutex_lock(&mutex);
        if (thread_id != count + 1)
            pthread_cond_wait(&cond[thread_id - 1], &mutex);
        printf("%d\n", thread_id);
        count = (count + 1) % total_threads;
        pthread_cond_signal(&cond[count]);
        pthread_mutex_unlock(&mutex);
        break; // remove this line to print infinitely
    }
    return 0;
}

int main(int argc, char *argv[])
{
    total_threads = atoi(argv[1]);

    pthread_t mythreads[total_threads];
    int mythread_id[total_threads];

    cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t) * total_threads);

    for (int i = 0; i < total_threads; i++)
    {
        pthread_cond_init(&cond[i], NULL);
        mythread_id[i] = i + 1;
        pthread_create(&mythreads[i], NULL, print, (void *)&mythread_id[i]);
    }

    for (int i = 0; i < total_threads; i++)
    {
        pthread_join(mythreads[i], NULL);
    }

    return 0;
}