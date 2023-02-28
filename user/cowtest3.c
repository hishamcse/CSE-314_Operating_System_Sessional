#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/memlayout.h"

// test 4
//  copyout() gets called for pipe and file I/O. So, your test code should do some file
// I/O or pipe operation before and after fork().

int fds[2];
char buf[4096];

void copyoutTest() {
    printf("copyout: ");

    buf[0] = 99;

    for (int i = 0; i < 4; i++) {
        if (pipe(fds) != 0) {
            printf("pipe() failed\n");
            exit(-1);
        }

        int pid = fork();
        if (pid < 0) {
            printf("fork failed\n");
            exit(-1);
        }
        if (pid == 0) {
            sleep(1);
            if (read(fds[0], buf, sizeof(i)) != sizeof(i)) {
                printf("error: read failed\n");
                exit(1);
            }
            sleep(1);
            int j = *(int *)buf;
            if (j != i) {
                printf("error: read worked but wrong value\n");
                exit(1);
            }
            exit(0);
        }
        if (write(fds[1], &i, sizeof(i)) != sizeof(i)) {
            printf("error: write failed\n");
            exit(-1);
        }
    }

    int xstatus = 0;
    for (int i = 0; i < 4; i++) {
        wait(&xstatus);
        if (xstatus != 0) {
            exit(1);
        }
    }

    if (buf[0] != 99) {
        printf("error: child overwrote parent\n");
        exit(1);
    }

    printf("test 4 copyout passed\n");
}

int main(int argc, char *argv[])
{
    copyoutTest();
    exit(0);
}