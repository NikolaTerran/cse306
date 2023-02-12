#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

//modified from user/cat.c

// already exist in sound.c?
// void
// beep(int freq, int duration)
// {

// }

int
main(int argc, char *argv[])
{
    if(argc < 2){
        printf(1,"less than 2 args\n");
        exit();
    }

    printf(1,"in user/beep.c\n");
    int *cast = (int*)argv;
    beep(cast[1],cast[2]);
    exit();
//   int fd, i;

//   if(argc <= 1){
//     cat(0);
//     exit();
//   }

//   for(i = 1; i < argc; i++){
//     if((fd = open(argv[i], 0)) < 0){
//       printf(1, "cat: cannot open %s\n", argv[i]);
//       exit();
//     }
//     cat(fd);
//     close(fd);
//   }
//   exit();
}
