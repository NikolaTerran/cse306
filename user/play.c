#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

struct sndpkt {
    uint frequency;   // frequency in millihertz
    uint duration;    // duration in milliseconds
};



//modified atoi from ulib.c
void
p_atoi(const char *s)
{
  struct sndpkt packets[8];
  int freq = 0;
  int duration = 0;
  int index = 0;

  //*s != 4 checks for ctrl-D
  while(*s != 0 && *s != 4 && index < 8){
    //ignores space and tabs
    while(('0' <= *s && *s <= '9') || *s == 32 || *s == 9){
      if(*s != 32 && *s != 9){
        freq = freq*10 + *s - '0';
      }
      s++;
    }
      
    //skip comma or newline
    s++;

    while(('0' <= *s && *s <= '9') || *s == 32 || *s == 9){
      if(*s != 32 && *s != 9){
        duration = duration*10 + *s - '0';
      }
      s++;
    }
      
    s++;

    packets[index].frequency = freq;
    packets[index].duration = duration;

    freq = 0;
    duration = 0;
    index += 1;
  }

  play(&packets[0]);
}

//copied from cat.c 's implementation
char buf[1024];

void uplay(int fd){
  int n;
  while((n = read(fd, buf, sizeof(buf))) > 0) {
    //assume the file is less than 1024 characters,
    //and its format is correct: freq, duration \n freq, duration...
    p_atoi(buf);
    exit();
  }
  if(n < 0){
    printf(1, "play: read error\n");
    exit();
  }
}

int
main(int argc, char **argv)
{

  int fd, i;

  if(argc <= 1){
    uplay(0);
    exit();
  }

  //taking filename from commandline args, not stdin
  for(i = 1; i < argc; i++){
    if((fd = open(argv[i], 0)) < 0){
      printf(1, "play: cannot open %s\n", argv[i]);
      exit();
    }
    uplay(fd);
    close(fd);
  }
  exit();
}