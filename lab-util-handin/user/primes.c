#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define endNum 35
#define startNum 2

void wrapper(int pips[2]){
    close(pips[1]);
    int newpips[2];
    int pid;
    uint32 curr;
    int stat = read(pips[0], &curr, sizeof(curr));
    if(!stat)
        return;
    pid = fork();
    if(pid == 0){
        printf("prime %d\n", curr);
        close(pips[0]);
    } else {
        wait((int*)0);
        int num;
        pipe(newpips);
        while(read(pips[0], &num, sizeof(num))){
            if(num % curr != 0)
                write(newpips[1], &num, sizeof(num));
        }
        close(newpips[1]);
        wrapper(newpips);
  //      wait(0);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    int pips[2]; 
    pipe(pips);
    for(uint32 i = startNum; i <= endNum; i++){
        write(pips[1], &i, sizeof(i));
    }
  
    wrapper(pips);
    
 //    printf("prime %d\n", curr);

    exit(0);
}