// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user.h"

#define NUM_DEVICES 3

struct TableElement {
    char* name;
    int major;
    int minor;
} ip_table[] = {
    {"console", 1,  1}, 
    {"/com1", 2,  1}, 
    {"/com2", 3, 2}};

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
  for(int i = 0; i < NUM_DEVICES; i++){
    if(fork()){
      int pid, wpid;
      if(open(ip_table[i].name, O_RDWR) < 0){
        mknod(ip_table[i].name, ip_table[i].major, ip_table[i].minor);
        open(ip_table[i].name, O_RDWR);
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
  exit();
}
