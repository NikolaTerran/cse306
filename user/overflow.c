#include "kernel/types.h"
#include "user.h"


/* TEST PROGRAM for stack overflow.
Running the following (infinitely recursive) test program will demonstrate 
what happens when a stack overflow occurs:
*/
void rec(int i) {
  printf(1, "%d(0x%x)\n", i, &i);
  rec(i+1);
}

int
main(int argc, char *argv[])
{
  rec(0);
}