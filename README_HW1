# CSE306 HW1

# Overview of the assignment
The primary goal of this assignment was to implement our own syscall, called play, that would play a sequence of tones over the speaker. Each tone is represented by a sndpkt struct, with fields for both frequency and duration.

We created several new files in order to have the play syscall work correctly in xv6. First, a new sound.c file was created in the kernel directory, in which the actual implementation of the play() syscall is located. This play function calls beep(), which is what it uses to produce sound. Beep accepts an int freq and int duration as its arguments, and a variable called dur_left that keeps track of the remaining duration of the sound. On each system timer interrupt, which we figured occurs about every 10ms, dur_left is decremented by 10. Once dur_left reaches a value less than 10, the tone is canceled by calling the function no_sound(). Because a timer interrupt occurs every 10ms, if the current duration less than 10ms, no sound would be able to be produced, since the minimum duration for producing some sort of noise is 10ms.

In the user directory, two new files called beep.c and play.c were created. beep.c, from exercise 2, is a demonstration program that allows the user to type in the command "beep" followed by frequency and duration values as its first and second two command-line arguments, and dispatches to the beep syscall. play.c is a demonstration program that allows the user to type in the command "play <file-name>", opens the file name using the open() function defined in xv6, parses the frequency and duration values in the file in order to convert them to integers, and dispatches to the play syscall implemented in the sound.c file.

# How to compile and run
First, boot up xv6. To do this, make sure you are in the top-level xv6 directory, and run "make" then "make qemu-nox". Once you are in xv6, create a new file as follows:

# file format for user play program:
csv format, no header, white spaces allowed.
first column is frequency, second column is duration.
limit to 8096 characters and 256 notes per file.

how to create a sound file:
```
$ cat > notes.csv
100,1000
200,1000
```
terminated with ctrl+d

how to use play program:
```
$ play notes.csv
$ play < notes.csv
$ play notes.csv &; play notes2.csv &; play notes3.csv
```

# Assumptions and design decisions
The play syscall in the kernel copies in packets from user space and places each packet in a buffer. The buffer is represented as a circular array of sndpkt structs, with a maximum capacity of 128 elements. The decision for using an array was that it seemed like the most viable option, as using something like a linked list would mean that we would have to keep track of the head pointer instead of being able to access an element through indexing. In addition, we did not want to deal with any sort of memory allocation of the sound packets. 

Packets placed in the buffer are consumed on each tick of the system clock. Beacuse the timer ticks occur every 10ms, multiple packets with durations less than 10 can be discarded. We assumed that if the total duration of all packets in the buffer was less than 10, no sound would get played at all. For example, the following would produce no sound, as the total duration of all packets in the buffer is 3ms:

buffer: [freq:1000, dur: 3], [freq:0, dur:0]

This makes sense, because for any sound to be produced at all, the timer must tick at least once and the ticks occur only every 10ms.

For the following example, a tone with frequency 1000Hz gets played for 10ms, the packet with frequency 2000Hz gets discarded, and a tone with frequency 3000Hz gets played for 93ms.

buffer: [freq:1000, dur: 1], [freq:2000, dur:2], [freq:3000, dur:100], [freq:0, dur:0]

Below is how we implemented the locks for buffer synchronization:

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


# questions from hw1 doc:

How does the xv6 kernel determine which system call was requested?

It knows the binding of the system call from usys.S file.

How is control dispatched to the kernel code that implements the requested system call?

In syscall.c. It determines the syscall number through the code num = curproc->tf->eax; and dispatches to the corresponding call through the code curproc->tf->eax = syscalls[num](); Specifically, For the beep and play syscalls, the syscall numbers are 22 and 23 respectively.

How does the xv6 kernel obtain the arguments to the system call?

It gets the argument with function such as argint, argptr and argstr. Which relies on the implementation of myproc().

How are system call results returned to the user process?

It is returned from corresponding syscall functions located in sysproc.c or sysfile.c
