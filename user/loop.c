#include "kernel/types.h"
#include "user.h"


/* TEST PROGRAM for stack overflow.
Running the following (infinitely recursive) test program will demonstrate 
what happens when a stack overflow occurs:
*/

int
main(int argc, char *argv[])
{ 
  int i = 0;
  while(1){
    i++;
    i++;
  }
  exit();
}