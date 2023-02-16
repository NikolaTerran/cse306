#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

struct sndpkt {
    uint frequency;   // frequency in millihertz
    uint duration;    // duration in milliseconds
};

int
main(int argc, char **argv)
{

  struct sndpkt packets[7];
  packets[0].frequency = 1000;
  packets[0].duration = 1000;

  packets[1].frequency = 600;
  packets[1].duration = 1000;

  packets[2].frequency = 1193;
  packets[2].duration = 1000;

  packets[3].frequency = 800;
  packets[3].duration = 1000;

  packets[4].frequency = 1000;
  packets[4].duration = 1000;

  packets[5].frequency = 900;
  packets[5].duration = 1000;

  packets[6].frequency = 0;
  packets[6].duration = 0;

  play(&packets[0]);

  /* 
  //Malloc also works
  struct sndpkt *packets = malloc(sizeof(struct sndpkt));
  packets->frequency = 1000;
  packets->duration = 1000;

  play(packets); */

/*
  //pre-defined packets structure
  struct sndpkt packets[1];
  packets[0].frequency = 500;
  packets[0].duration = 1000;
  play(packets);
  packets[0].frequency = 1000;
  packets[0].duration = 1000;
  play(packets);
  packets[0].frequency = 1500;
  packets[0].duration = 1000;
  play(packets);
  packets[0].frequency = 2000;
  packets[0].duration = 1000;
  play(packets);
  packets[0].frequency = 2500;
  packets[0].duration = 1000;
  play(packets);
  packets[0].frequency = 500;
  packets[0].duration = 1000;
  play(packets);
  packets[0].frequency = 1000;
  packets[0].duration = 1000;
  play(packets);
//   send this to sys_play to stop playing
//   packets[0].frequency = 0;
//   packets[0].duration = 0;
//   play(packets);
*/

  exit();
}