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

# spinlock implementation
define struct spinlock buflock in sound.c
add the definition to def.h
init it in main.c
acquire/release it in sound.c

# file format for user play program:
csv format, no header, white spaces allowed.
first column is frequency, second column is duration.
limit to 8096 characters and 256 notes per file.

how to create a sound file:
```
$ cat > notes.csv
100,1000
```
terminated with ctrl+d

how to use play program:
```
$ play notes.csv
$ play < notes.csv
$ play notes.csv &; play notes2.csv &; play notes3.csv
```

# questions from hw1 doc:

How does the xv6 kernel determine which system call was requested?

It knows the binding of the system call from usys.S file.

How is control dispatched to the kernel code that implements the requested system call?

In syscall.c

How does the xv6 kernel obtain the arguments to the system call?

It gets the argument with function such as argint, argptr and argstr. Which relies on the implementation of myproc().

How are system call results returned to the user process?

It is returned from corresponding syscall functions located in sysproc.c or sysfile.c

