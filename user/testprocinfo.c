#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/pstat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    printf("\nStatistical information about the in use processes\n");
    struct pstat ps;
    getpinfo(&ps);
    printf("\nPID | In Use | Original Tickets | Current Tickets | Time Slices\n");

    for (int i = 0; i < NPROC; i++) {
        if (ps.inuse[i] == 1) {
            printf("%d\t%d\t\t%d\t\t%d\t\t%d\n", ps.pid[i], ps.inuse[i], ps.tickets_original[i], ps.tickets_current[i], ps.time_slices[i]);
        }
    }

    exit(0);
}