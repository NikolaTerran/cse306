#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

int
main(int argc, char **argv)
{

  if(argc < 3){
    printf(2, "Not enough args (need freq and duration) \n");
    exit();
  }
  beep(atoi(argv[1]), atoi(argv[2]));

  exit();
}