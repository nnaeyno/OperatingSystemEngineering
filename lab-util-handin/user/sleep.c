
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {

  char *ticks;

  if(argc != 2){
    fprintf(2, "Usage: sleep files...\n");
    exit(1);
  }
  ticks = argv[1];
  sleep(atoi(ticks));

  exit(0);
}
