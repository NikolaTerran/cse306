// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
   
  
  //The proper device node has to be opened and dup() used to set up stdin, stdout, and stderr each time. 
  //You will need to generalize the code in init to accomplish this. 
  //Perhaps the best way to do it would be to define a table, 
  //where each entry gives the name of a device node together with its major and minor device numbers. 
  //When init starts, it could iterate through this table, 
  //ensuring that each device node exists and starting a shell to run on that device node. 
  int pid, wpid;

  if(fork()){
    if(open("console", O_RDWR) < 0){
      mknod("console", 1, 1);
      open("console", O_RDWR);
    }
    dup(0);  // stdout
    dup(0);  // stderr

    for(;;){
      printf(1, "init: starting sh\n");
      pid = fork();
      if(pid < 0){
        printf(1, "init: fork failed\n");
        exit();
      }
      if(pid == 0){
        exec("sh", argv);
        printf(1, "init: exec sh failed\n");
        exit();
      }
      while((wpid=wait()) >= 0 && wpid != pid)
        printf(1, "zombie!\n");
    }
  }else{
    //So to be able to start a shell on COM1, 
    //it is necessary to ensure that there is a device node (call it "/com1"), 
    //with major device number 2 (for which you defined the constant UART in the kernel) and minor device number 1.
    if(fork()){
      if(open("/com1", O_RDWR) < 0){
        mknod("/com1", 2, 1);
        open("/com1", O_RDWR);
      }
      dup(0);  // stdout
      dup(0);  // stderr

      for(;;){
        printf(1, "init: starting sh\n");
        pid = fork();
        if(pid < 0){
          printf(1, "init: fork failed\n");
          exit();
        }
        if(pid == 0){
          exec("sh", argv);
          printf(1, "init: exec sh failed\n");
          exit();
        }
        while((wpid=wait()) >= 0 && wpid != pid)
          printf(1, "zombie!\n");
      }
    }else{
      //Then init needs to fork twice, once for the shell to be started on /console 
      //and another time for the shell to be started on /com1. 
      if(open("/com2", O_RDWR) < 0){
        mknod("/com2", 3, 1);
        open("/com2", O_RDWR);
      }
      dup(0);  // stdout
      dup(0);  // stderr

      for(;;){
        printf(1, "init: starting sh\n");
        pid = fork();
        if(pid < 0){
          printf(1, "init: fork failed\n");
          exit();
        }
        if(pid == 0){
          exec("sh", argv);
          printf(1, "init: exec sh failed\n");
          exit();
        }
        while((wpid=wait()) >= 0 && wpid != pid)
          printf(1, "zombie!\n");
      }
    }
    
  }
  
}
