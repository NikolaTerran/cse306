
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
x_2 = c * x_1 + 0 = c * c * 99

to find 60th iteration:
c^60 * 99 = 10
so c is (10/99)'s 60th root
c is approximately 0.96

# part 3

use the same formula as part 2.
replace sample with (p->running - p->last_run)/10.0

constant calculation:
c^30 * 99 = 10
c is approximately 0.93