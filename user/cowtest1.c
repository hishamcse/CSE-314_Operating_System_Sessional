#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// test 1 & 2
// Write a user code to check if fork() system-call works and handles a write operation correctly

int main(int argc, char *argv[])
{
    int s = 1024;
    char *p = sbrk(s);

    if (p == (char *)0xffffffffffffffffL)
    {
        printf("sbrk(%d) failed\n", s);
        exit(-1);
    }

    for (char *q = p; q < p + s; q += 4096)
    {
        *(int *)q = getpid();
    }

    int pid = fork();
    if (pid < 0)
    {
        printf("fork() failed\n");
        exit(-1);
    }

    if (pid == 0)
        exit(0);

    wait(0);

    if (sbrk(-s) == (char *)0xffffffffffffffffL)
    {
        printf("sbrk(-%d) failed\n", s);
        exit(-1);
    }

    printf("test 1 & 2 ok\n");
    exit(0);
}
