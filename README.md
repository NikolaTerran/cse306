# cse306
repo for cse306 homeworks

# todo:
1. investigate while loop inside beep in sound.c (side effect of cprintf)
2. resolve concurrency/race condition for play syscall?
3. sleep lock/ spin lock?
4. make play.c able to process large file?

# playlock(sleeplock) implementation
define struct sleeplock playlock in sound.c
add the definition to def.h
init it in main.c
acquire/release it in sys_play