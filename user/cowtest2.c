#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// test 3
// Write a user code to check if fork() system-call works and a COW page is freed when both parent and child process
// writes to it. Write a system call that prints the statistics for page usage: the number of pages
// being used by each running process and the number of pages in freelist

void test3()
{
    printf("Initial Statistics:\n");
    statPage();

    char *p = sbrk(4096);
    if (p == (char *)0xffffffffffffffffL)
    {
        printf("sbrk(%d) failed\n", 4096);
        exit(-1);
    }

    statPage();

    *p = '1';
    printf("PID %d: Parent writes: %c\n", getpid(), *p);
    statPage();

    int pid = fork();
    if (pid < 0)
    {
        printf("fork() failed\n");
        exit(1);
    }

    if (pid == 0)
    {
        printf("PID %d: Child starts: %c\n", getpid(), *p);
        statPage();

        *p = '2';
        printf("PID %d: Child writes: %c\n", getpid(), *p);
        statPage();
        exit(0);
    }
    else
    {
        wait(0);
        printf("PID %d: Parent finishes: %c\n", getpid(), *p);
        statPage();
    }

    printf("Final Statistics:\n");
    statPage();

    printf("Test 3 page statistics: OK\n");
}

int main(int argc, char *argv[])
{
    test3();
    exit(0);
}
