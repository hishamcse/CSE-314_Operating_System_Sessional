#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <wait.h>
#include <pthread.h>

pthread_mutex_t lock;
pthread_cond_t barber, customer;

int num_chairs, num_customers, num_customers_served, waiting_customers;

void *barber_func(void *data)
{

    while (1)
    {
        pthread_mutex_lock(&lock);
        while (waiting_customers == 0 && num_customers_served < num_customers)
        {
            pthread_cond_wait(&customer, &lock);
        }
        waiting_customers--;
        num_customers_served++;
        printf("Barber is cutting hair\n");
        pthread_cond_signal(&barber);
        pthread_mutex_unlock(&lock);

        if (num_customers_served == num_customers)
        {
            return 0;
        }
    }
    return 0;
}

void *customer_func(void *data)
{
    int thread_id = *((int *)data);

    pthread_mutex_lock(&lock);

    if (waiting_customers >= num_chairs)
    {
        printf("Customer %d is leaving and will leave bad review\n", thread_id);
        num_customers_served++;
        pthread_mutex_unlock(&lock);
        return 0;
    }

    waiting_customers++;
    printf("Customer %d is waiting\n", thread_id);
    pthread_cond_signal(&customer);
    pthread_cond_wait(&barber, &lock);
    pthread_mutex_unlock(&lock);

    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    pthread_t barber_thread, *customer_threads;

    if (argc != 3)
    {
        printf("Usage: ./barber <num_chairs> <num_customers>");
        exit(1);
    }

    num_chairs = atoi(argv[1]);
    num_customers = atoi(argv[2]);
    num_customers_served = 0;
    waiting_customers = 0;

    printf("Number of chairs: %d\n", num_chairs);
    printf("Number of customers: %d\n", num_customers);

    customer_threads = (pthread_t *)malloc(sizeof(pthread_t) * num_customers);

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&barber, NULL);
    pthread_cond_init(&customer, NULL);

    pthread_create(&barber_thread, NULL, barber_func, NULL);

    for (i = 0; i < num_customers; i++)
    {
        int *thread_id = (int *)malloc(sizeof(int));
        *thread_id = i;
        pthread_create(&customer_threads[i], NULL, customer_func, (void *)thread_id);
    }

    pthread_join(barber_thread, NULL);
    printf("Barber is done\n");

    for (i = 0; i < num_customers; i++)
    {
        pthread_join(customer_threads[i], NULL);
        // printf("Customer %d is done\n", i);
    }

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&barber);
    pthread_cond_destroy(&customer);

    free(customer_threads);

    return 0;
}