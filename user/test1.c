#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/stat.h"
#include "user/user.h"

// test 1
// This is a good place to test your implementation. Write a user code that uses some
// number of pages provided by the command line. Make sure the user code takes some
// time to execute. Write a system-call that prints the number of live pages being used by
// different processes. Now run multiple instances of the user code from shell (you know
// this from first xv6 offline) and use the system-call to check if the counts match.

void test1(int n)
{
    int i;
    int *p = malloc(n * sizeof(int));
    for (i = 0; i < n; i++)
    {
        p[i] = i;
    }
    for (i = 0; i < n; i++)
    {
        // printf("p[%d] = %d  ", i, p[i]);
    }
    printf("n = %d\n", n);

    free(p);
    statLivePages();
}

int main(int argc, char *argv[])
{
    int num_pages = atoi(argv[1]);

    char *pages[num_pages];
    for (int i = 0; i < num_pages; i++) {
        pages[i] = sbrk(4096);
        printf("Allocated page %d at %p\n", i, pages[i]);
    }

    // Use the pages for some computation
    int sum = 0;
    for (int i = 0; i < num_pages * 4096 / sizeof(int); i++) {
        sum += i;
    }

    printf("Sum: %d\n", sum);

    // statLivePages();

    for (int i = 0; i < num_pages; i++) {
        sbrk(-4096);
    }

    exit(0);
}
