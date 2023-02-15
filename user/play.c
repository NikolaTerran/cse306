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
  exit();
}