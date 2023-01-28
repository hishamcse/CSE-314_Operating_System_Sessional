#include <pthread.h>

typedef struct zemaphore
{
    int value;             // value of the zemaphore
    pthread_mutex_t mutex; // mutex lock for this structure
    pthread_cond_t cond;   // condition variable for this structure
} zem_t;

void zem_init(zem_t *, int);
void zem_up(zem_t *);
void zem_down(zem_t *);
