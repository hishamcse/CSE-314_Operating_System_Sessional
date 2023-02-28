#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// test 1
// This is a good place to test your implementation. Write a user code that uses some
// number of pages provided by the command line. Make sure the user code takes some
// time to execute. Write a system-call that prints the number of live pages being used by
// different processes. Now run multiple instances of the user code from shell (you know
// this from first xv6 offline) and use the system-call to check if the counts match.

void test()
{
    statLivePages();
}

int main(int argc, char *argv[])
{

    test();
    exit(0);
}
