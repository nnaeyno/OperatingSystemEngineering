#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"
#define MY_MAX 512


int main(int argc, char *argv[]) {
    if(argc < 2)
        exit(1);
    int pid;
    char* toPass[MAXARG];
    char otherArg[MY_MAX];
    int r = 0;
    char ch;
    for(int i = 0; i < argc-1; i++){
        toPass[i] = argv[i+1];
    }

    while(read(0, &ch, 1)){
        if(ch == '\n'){
            otherArg[r] = '\0';
            pid = fork();
            if(pid){
                wait(0);
            } else {
                toPass[argc-1] = otherArg;
                toPass[argc] = 0;
                exec(argv[1], toPass);
            }
            r = 0;
        } else {
            otherArg[r] = ch;
            r++;
        }
    }

   
    exit(0);
}

