#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
   // without fork
   printf("testticket without fork\n");
   int tickets = atoi(argv[1]);
   settickets(tickets);

   while (1) { /* code */}
   exit(0);

//    // with fork
//    printf("testticket with fork\n");
//    int tickets = atoi(argv[1]);
//    settickets(tickets);

//    int rc = fork();
//     if (rc < 0) {
//         printf("fork failed\n");
//         exit(1);
//     } else if (rc == 0) {
//         // child
//         // printf("Child process\n");
//         while (1) { /* code */}
//         exit(0);
//     } else {
//         // parent
//         // printf("Parent process\n");
//         while (1) { /* code */}
//         exit(0);
//     }
}