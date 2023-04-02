# CSE306 HW2

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
replace sample with (p->running - p->last_run)/10.0

constant calculation: (exponential averaging of 3000 ticks)
c^30 * 99 = 10
c is approximately 0.93

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

stats after 20 cycles:
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


lowestwait%:
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