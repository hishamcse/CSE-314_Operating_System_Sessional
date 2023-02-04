#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>
#include "zemaphore.h"

#define MAX_CUSTOMERS 10

int total_chairs;
int waiting_customers;
zem_t barber;
zem_t customer;
zem_t mutex;

void *barber_func(void *data)
{
    while (1)
    {
        zem_down(&customer);
        zem_down(&mutex);
        waiting_customers--;
        zem_up(&barber);
        zem_up(&mutex);
        printf("Barber: Cutting hair\n");
        if (waiting_customers == 0)
            break;
    }
    return 0;
}

void *customer_func(void *data)
{
    int id = *((int *)data);
    // while (1)
    // {
    zem_down(&mutex);
    if (waiting_customers < total_chairs)
    {
        waiting_customers++;
        printf("Customer %d: Waiting in the waiting room for barber\n", id);
        zem_up(&customer);
        zem_up(&mutex);
        zem_down(&barber);
        printf("Customer %d: Getting haircut\n", id);
    }
    else
    {
        zem_up(&mutex);
        printf("Customer %d: Leaving the barber shop because waiting room is full\n", id);
    }
    // }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: ./barber <number of chairs in waiting room> \n");
        exit(1);
    }

    // printf("Barber shop problem with semaphores\n");

    total_chairs = atoi(argv[1]);
    waiting_customers = 0;

    zem_init(&barber, 0);
    zem_init(&customer, 0);
    zem_init(&mutex, 1);

    pthread_t barber_thread;
    pthread_create(&barber_thread, NULL, barber_func, NULL);

    pthread_t customer_threads[MAX_CUSTOMERS];
    int customer_ids[MAX_CUSTOMERS];
    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        customer_ids[i] = i;
        pthread_create(&customer_threads[i], NULL, customer_func, (void *)&customer_ids[i]);
    }

    pthread_join(barber_thread, NULL);
    for (int i = 0; i < MAX_CUSTOMERS; i++)
    {
        pthread_join(customer_threads[i], NULL);
    }

    // printf("Main thread sleeping for %d seconds\n", 2);
    // sleep(2);
    // /* 6. Exit.  */
    // printf("Main thread exiting\n");
    // exit(0);

    return 0;
}