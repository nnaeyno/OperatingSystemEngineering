
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


int main(int argc, char *argv[]) {
    int pid, pips[2];
    char byte = 'x';
    pipe(pips);
    pid = fork();

    if(pid == 0) {//child
        read(pips[0], &byte, sizeof(byte));
        printf("%d: received ping\n", pid);
        write(pips[1], &byte, 1);
      
    } else {//parent
        write(pips[1], &byte, 1);
        wait((int *) 0);
        read(pips[0], &byte, sizeof(byte));
        printf("%d: received pong\n", pid);
    }
    exit(0);
}
/*
   
*/