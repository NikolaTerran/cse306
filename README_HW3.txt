# CSE306 HW2

Changes to xv6/Makefile.inc: added the line "CFLAGS += -DQUANTUM=100" as stated in exercise 6.

Created new files in xv6/user directory (loop.c, infloop.c, part3test.c, and l1quant.c) to test out the different schedulers in exercise 7. Changed xv6/user/Makefile to link these new files accordingly so that they can run in xv6 via command line.

To see the outputs for each exercise, see the README_HW3_figures.docx file, which contains screenshots. (Need to open this file using a word processing program such as Microsoft Word or Apple Pages.)

# part 0

No changes were made to calibrate the timer interrupts. The default local APIC timer produces interrupts at a rate of approximate 100Hz, i.e. 100 ticks per second.

# part 1

Introduced three new fields in the proc structure: running, runnable, and sleeping. Depending on the test the process is in at each tick, the appropriate field gets incremented. This is done in the function incrementstats(), which is called in the trap handler. xv6 then produces printouts on the console every 1000 ticks, or approximately every 10 seconds.

# part 2 

derivation for c:

avg = c * avg + (1-c) * sample
"if the samples were to suddenly become zero at some point and stay that way, then after 60 seconds have passed the original average will have decayed to 1/10 of its initial value."

lets say initial avg is 99, we want avg after 60 iterations become 1/10 * avg

0th iteration:
99 = c * 99 + (1-c) * 1 = 100

1st iteration:
x_1 = c * 99 + (1-c) * 0 = c * 99

2nd iteration:
x_2 = c * x_1 + 0 = c * c * 99 = c^2 * 90

to find 60th iteration:
c^60 * 99 = 10
so c is (10/99)'s 60th root
c is approximately 0.96

# part 3

use the same formula as part 2.
replace sample with (p->running - p->last_run)/100.0

constant calculation: (exponential averaging of 3000 ticks)
c^30 * 99 = 10
c is approximately 0.93

# part 4

Used the same exponential averaging formula as in part 2, and constant is the same as part 3, c = 0.93. Replace sample with (p->runnable - p->last_wait)/100.0. A sample gives the total number of ticks a process is waiting during a 100 tick interval.


# part 5

For each process, records time at which the process transitions to the runnable state to time at which the process transitions to the running state. This is done in calc_latency(), which is called at each timer interrupt. 

Takes the maximum of all wakeup latencies occurring over a 100 tick interval and uses that as the sample. Uses the same exponential averaging formula as in part 2, and the constant is the same as part 3, c = 0.93

We saw that when very few processes are being run concurrently, the latency for each process is approximately zero, since xv6 invokes the scheduler during each tick. However, when there are many processes running concurrently, the latencies become a non-zero number.

# part 6

QUANTUM=100. During each timer interrupt, checks if CPU is currently idle by calling mycpu() and accessing the field proc. If proc is null, the CPU is currently idle and reschedule() can be called; otherwise there is currently a process scheduled on the CPU. This procedure is done to avoid letting a runnable process languish in the CPU queue until the next scheduling interval (after 100 ticks) even if there is an idle CPU that could run that process.

If the process' CPU burst is less than one quantum (100 ticks), then CPU will become idle at some point before 100 ticks and thus reschedule() can be called before 100 ticks. If process' CPU burst is greater than one quantum, then it will run for 100 ticks, then get preempted due to reschedule() being called, since the statement t % QUANTUM = 0 becomes true.

# part 7

cpu bound processes: infloop
io bound processes: sh
processes span less than one quantum: l1quant
processes span more than one quantum: infloop 

roundrobin (quantum=100):
when infloop is running, the io bound processes and processes span less than 1 quantum feels 
very unresponsive. The delay between enter l1quant and lorem ipsum text showing up will take seconds.

avg delay between issuing l1quant and text show up when 2 infloop instances are running:
(1.9 + 1.33 + 1.9 + 3.15 + 3.60)/5
=2.376 seconds

stats after 20 cycles:
$ cpus: 1, uptime: 20000, load(x100): 215, scheduler: roundrobin
1 sleep init run: 1 wait: 12 sleep: 19987 cpu%: 0 wait%: 0 latency: 0 
2 sleep init run: 1 wait: 1 sleep: 19986 cpu%: 0 wait%: 0 latency: 0 
3 sleep init run: 0 wait: 3 sleep: 19984 cpu%: 0 wait%: 0 latency: 0 
5 sleep sh run: 7 wait: 741 sleep: 19239 cpu%: 0 wait%: 0 latency: 0 
6 sleep sh run: 1 wait: 0 sleep: 19985 cpu%: 0 wait%: 0 latency: 0 
7 sleep sh run: 1 wait: 1 sleep: 19984 cpu%: 0 wait%: 0 latency: 0
10 run infloop run: 9485 wait: 9724 sleep: 0 cpu%: 50 wait%: 49 latency: 0 
12 runble infloop run: 9600 wait: 9603 sleep: 5 cpu%: 48 wait%: 51 latency: 0

turn around time for console sh: (7 + 741)/7 = 107
turn around time for both infloop: 2


lowestcpu%:
when multiple instances of infloop are running, the sh is quite slow to respond to the user input,
but l1quant prints faster than the round-robin scheduler.

avg delay between issuing l1quant and text show up when 2 infloop instances are running:
(2.32 + 0.92 + 0.49 + 0.71 + 0.94)/5
= 1.076 seconds

stats after (approximately) 20 cycles:
cpus: 1, uptime: 21000, load(x100): 200, scheduler: lowest CPU% first
1 sleep init run: 1 wait: 12 sleep: 20987 cpu% : 0 wait%: latency: 0
2 sleep init run: 0 wait:1 sleep: 20987 cpu%: 0 wait%: 0 latency: 0
3 sleep init run: 1 wait: 548 sleep: 20986 cpu%: 0 wait%: 0 latency: 0
4 sleep sh run: 4 wait: 548 sleep: 20435 cpu%: 0 wait%: 0 latency: 0
5 sleep sh run: 1 wait: 1 sleep: 20985 cpu%: 0 wait%: 0 latency: 0
6 sleep sh run: 2 wait: 1 sleep: 20983 cpu%: 0 wait%: 0 latency: 0
10 runble infloop run: 10115 wait: 10278 sleep: 0 cpu%: 48 wait%: 51 latency: 0
12 run infloop run: 10149 wait: 10238 sleep: 5 cpu%: 51 wait%: 48 latency: 0

turn around time for console sh: (4 + 548)/4 = 138
turn around time for both infloop: 2


highestwait%:
when multiple instances of infloop are running, it is slowest to respond to any user
input or run another processes that's less than 1 quantum.

avg delay between issuing l1quant and text show up when 2 infloop instances are running:
(8.72 + 15.44 + 12.57 + 14.42 + 16.97)/5 
= 13.624  seconds

stats after 20 cycles:
cpus: 1, uptime: 20000, load(x100): 264, scheduler: highest wait first 
1 sleep init run: 1 wait: 112 sleep: 19887 cpu%: 0 wait%: 0 latency: 0 
2 sleep init run: 0 wait: 89 sleep: 19800 cpu%: 0 wait%: 0 latency: 0
3 sleep sh run: 1 wait: 7255 sleep: 12631 cpu%: 0 wait%: 35 latency: 0
4 sleep init run: 0 wait: 101 sleep: 19699 cpu%: 0 wait%: 0 latency: 0
5 sleep sh run: 1 wait: 100 sleep: 19699 cpu%: 0 wait%: 0 latency: 0 
7 sleep sh run: 1 wait: 100 sleep: 19599 cpu%: 0 wait%: 0 latency: 0 
13 run infloop run: 9331 wait: 9466 sleep: 0 cpu%: 51 wait%: 48 latency: 0 
15 runble infloop run: 9266 wait: 9525 sleep: 5 cpu%: 48 wait%: 51 latency: 0

turn around time for console sh: (1 + 7255)/1 = 7256 
turn around time for both infloop: 2

-------------

loop computes and sleeps for about the same number of ticks. 

roundrobin: (quantum=100)
stats after 20 cycles:
cpus: 1, uptime: 20000, load(x100): 143, scheduler: roundrobin 
1 sleep init run: 4 wait: 14 sleep: 19982 cpu%: 0 wait%: 0 latency: 0 
2 sleep init run: 3 wait: 2 sleep: 19981 cpu%: 0 wait%: 0 latency: 0
3 sleep sh run: 1 wait: 15 sleep: 19969 cpu%: 0 wait%: 35 latency: 0
4 sleep init run: 6 wait: 15 sleep: 19961 cpu%: 0 wait%: 0 latency: 0
5 sleep sh run: 3 wait: 5 sleep: 19973 cpu%: 0 wait%: 0 latency: 0 
7 sleep sh run: 4 wait: 7 sleep: 19969 cpu%: 0 wait%: 0 latency: 0 
11 run loop run: 8239 wait: 5964 sleep: 5462 cpu%: 40 wait%: 48 latency: 2 
10 runble loop run: 8249 wait: 5952 sleep: 5465 cpu%: 41 wait%: 51 latency: 2

turn around time for infloop: ~1.7


lowestcpu%:
stats after 20 cycles:
cpus: 1, uptime: 20000, load(x100): 142, scheduler: lowest CPU% first
1 sleep init run: 4 wait: 13 sleep: 19983 cpu%: 0 wait%: 0 latency: 0 
2 sleep init run: 2 wait: 2 sleep: 19982 cpu%: 0 wait%: 0 latency: 0
3 sleep sh run: 3 wait: 3 sleep: 19979 cpu%: 0 wait%: 35 latency: 0
4 sleep init run: 4 wait: 15 sleep: 19964 cpu%: 0 wait%: 0 latency: 0
5 sleep sh run: 3 wait: 4 sleep: 19975 cpu%: 0 wait%: 0 latency: 0 
7 sleep sh run: 3 wait: 7 sleep: 19970 cpu%: 0 wait%: 0 latency: 0 
11 run loop run: 7924 wait: 6538 sleep: 4922 cpu%: 40 wait%: 30 latency: 2 
10 runble loop run: 7913 wait: 6287 sleep: 5186 cpu%: 40 wait%: 35 latency: 2

turn around time for infloop: ~1.8


highestwait%:
stats after 20 cycles:
cpus: 1, uptime: 20000, load(x100): 152, scheduler: highest wait% first
1 sleep init run: 5 wait: 112 sleep: 19883 cpu%: 0 wait%: 0 latency: 0 
2 sleep init run: 2 wait: 85 sleep: 19798 cpu%: 0 wait%: 0 latency: 0
3 sleep sh run: 5 wait: 99 sleep: 19779 cpu%: 0 wait%: 35 latency: 0
4 sleep init run: 2 wait: 104 sleep: 19694 cpu%: 0 wait%: 0 latency: 0
5 sleep sh run: 3 wait: 99 sleep: 19696 cpu%: 0 wait%: 0 latency: 0 
7 sleep sh run: 4 wait: 99 sleep: 19596 cpu%: 0 wait%: 0 latency: 0 
11 run loop run: 7895 wait: 6433 sleep: 5124 cpu%: 40 wait%: 32 latency: 2 
10 runble loop run: 7972 wait: 6438 sleep: 5090 cpu%: 40 wait%: 32 latency: 2

turn around time for infloop: ~1.8

Overall, when running two loop processes concurrently, all three schedulers have about the same performance.

-------------

Running tests with "process mixes" (6 loop processes, 2 infloop processes):

roundrobin: (quantum=100)
stats after 20 cycles:
cpus: 1, uptime: 20000, load(x100): 799, scheduler: roundrobin 
1 sleep init run: 4 wait: 14 sleep: 19982 cpu%: 0 wait%: 0 latency: 0 
2 sleep init run: 3 wait: 1 sleep: 19981 cpu%: 0 wait%: 0 latency: 0
3 sleep sh run: 1 wait: 12 sleep: 19971 cpu%: 0 wait%: 0 latency: 0
5 sleep init run: 4 wait: 20 sleep: 19958 cpu%: 0 wait%: 0 latency: 0
6 sleep sh run: 4 wait: 2 sleep: 19974 cpu%: 0 wait%: 0 latency: 0 
7 sleep sh run: 3 wait: 6 sleep: 19971 cpu%: 0 wait%: 0 latency: 0 
19 runble loop run: 493 wait: 17692 sleep: 131 cpu%: 1 wait%: 97 latency: 93 
10 runble loop run: 530 wait: 17693 sleep: 133 cpu%: 1 wait%: 97 latency: 93
12 runble loop run: 526 wait: 17690 sleep: 138 cpu%: 1 wait%: 97 latency: 93
14 runble loop run: 518 wait: 17698 sleep: 136 cpu%: 1 wait%: 97 latency: 93
16 runble loop run: 539 wait: 17675 sleep: 135 cpu%: 1 wait%: 97 latency: 93 
18 runble loop run: 518 wait: 17657 sleep: 130 cpu%: 1 wait%: 97 latency: 93 
23 runble infloop run: 6559 wait: 10749 sleep: 90 cpu%: 36 wait%: 63 latency: 0 
22 runble infloop run: 8664 wait: 8714 sleep: 0 cpu%: 51 wait%: 48 latency: 0

turn around time for loop: ~35
turn around time for infloop: ~2.5
Out of all three schedulers, the roundrobin scheduler has the greatest latency for the loop processes (93 ticks). It essentially prioritizes the infloop processes, thus delaying the loop processes from transitioning to the running state and having them wait for long periods of time.



lowestcpu%:
stats after 20 cycles:
cpus: 1, uptime: 20000, load(x100): 799, scheduler: lowest CPU% first 
1 sleep init run: 4 wait: 13 sleep: 19982 cpu%: 0 wait%: 0 latency: 0 
2 sleep init run: 3 wait: 2 sleep: 19981 cpu%: 0 wait%: 0 latency: 0
3 sleep sh run: 2 wait: 3 sleep: 19971 cpu%: 0 wait%: 0 latency: 0
5 sleep init run: 7 wait: 18 sleep: 19958 cpu%: 0 wait%: 0 latency: 0
6 sleep sh run: 3 wait: 4 sleep: 19974 cpu%: 0 wait%: 0 latency: 0 
7 sleep sh run: 3 wait: 6 sleep: 19971 cpu%: 0 wait%: 0 latency: 0 
19 runble loop run: 2254 wait: 15759 sleep: 131 cpu%: 10 wait%: 85 latency: 35 
10 runble loop run: 2263 wait: 15868 sleep: 133 cpu%: 12 wait%: 84 latency: 51
12 runble loop run: 2243 wait: 15814 sleep: 138 cpu%: 10 wait%: 85 latency: 32
14 runble loop run: 2250 wait: 15882 sleep: 136 cpu%: 10 wait%: 85 latency: 57
16 runble loop run: 2259 wait: 15758 sleep: 135 cpu%: 12 wait%: 82 latency: 45 
18 runble loop run: 2244 wait: 15720 sleep: 130 cpu%: 12 wait%: 83 latency: 39 
23 runble infloop run: 2631 wait: 15458 sleep: 90 cpu%: 13 wait%: 86 latency: 0 
22 runble infloop run: 2748 wait: 15352 sleep: 0 cpu%: 17 wait%: 82 latency: 0

turn around time for loop: ~8
turn around time for infloop: ~6.5
As can be seen, the cpu utilization percentages are more uniform, as opposed to the roundrobin scheduler. This is because the lowest cpu% first scheduler prioritizes processes whose cpu% are smallest. As a result, the wakeup latency values for the loop processes are thus not as big as those of the roundrobin scheduler, as these processes are now being chosen to run more often than before.


highestwait%:
stats after 20 cycles:
cpus: 1, uptime: 20000, load(x100): 765, scheduler: highest wait% first
1 sleep init run: 3 wait: 112 sleep: 19885 cpu%: 0 wait%: 0 latency: 0 
2 sleep init run: 3 wait: 87 sleep: 19797 cpu%: 0 wait%: 0 latency: 0
3 sleep sh run: 7 wait: 3336 sleep: 16542 cpu%: 0 wait%: 0 latency: 0
4 sleep init run: 3 wait: 103 sleep: 19693 cpu%: 0 wait%: 0 latency: 0
5 sleep sh run: 4 wait: 100 sleep: 19693 cpu%: 0 wait%: 0 latency: 0 
7 sleep sh run: 4 wait: 97 sleep: 19596 cpu%: 0 wait%: 0 latency: 0 
11 runble infloop run: 5777 wait: 13559 sleep: 0 cpu%: 17 wait%: 82 latency: 0 
10 runble infloop run: 5659 wait: 13640 sleep: 0 cpu%: 14 wait%: 85 latency: 0
23 runble loop run: 780 wait: 11443 sleep: 267 cpu%: 10 wait%: 85 latency: 60 
14 runble loop run: 1650 wait: 12982 sleep: 668 cpu%: 10 wait%: 84 latency: 37
16 runble loop run: 1624 wait: 12017 sleep: 558 cpu%: 11 wait%: 83 latency: 60
18 runble loop run: 1496 wait: 11355 sleep: 545 cpu%: 11 wait%: 83 latency: 50
20 run loop run: 1310 wait: 10837 sleep: 445 cpu%: 11 wait%: 83 latency: 42
22 runble loop run: 989 wait: 9462 sleep: 342 cpu%: 11 wait%: 84 latency: 47

turn around time for loop: ~9
turn around time for infloop: ~3.3
As can be seen, the wait percentages are more uniform, as opposed to the roundrobin scheduler. This is because the highest wait% first prioritizes processes whose wait% are largest. As a result, the wakeup latency values for the loop processes are thus not as big as those of the roundrobin scheduler.
The only downside to this scheduler is that because it tries to allow each process to have a fair wait%, xv6 takes longer to boot up as opposed to the other schedulers. In addition, when introducing more processes to the system or when killing processes, it takes much longer.


