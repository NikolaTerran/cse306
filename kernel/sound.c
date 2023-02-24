#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "sound.h"
#include "spinlock.h"
#include "sleeplock.h"

struct sleeplock playlock;
struct spinlock buflock;

//Play sound using built in speaker
static void play_sound(uint nFrequence) {
	uint Div;
	uchar tmp;

	//Set the PIT to the desired frequency
	Div = 1193180 / nFrequence;
	outb(0x43, 0xb6);
	outb(0x42, (Div) & 0xFF);
	outb(0x42, (Div >> 8) & 0xFF);

	//And play the sound using the PC speaker
	tmp = inb(0x61);
	if ((tmp & 0x3) != 0x3) {
		outb(0x61, tmp | 3);
	}

	cprintf("Play sound done\n");
 }
 
 //make it shutup
 static void nosound() {
 	uchar tmp = inb(0x61) & 0xFC;
 
 	outb(0x61, tmp);
 }

//From hw1-doc:
//Note that to time the duration of a tone, 
//you will need to arrange for a function in your sound.c file to be called
// on each system timer interrupt. 
//The remaining duration of any tone that is playing 
//should be maintained in a variable that is counted down on each timer interrupt. 
//When the value reaches zero, the tone should be canceled. 
static int dur_left;

//LAPIC timers are set to interrupt periodically (about 100Hz).
// about every 0.01 seconds
// we want the duration specified in miliseconds (0.001)
// therefore we decrement dur_left by a factor of 10
int beep_timer() {

	if (dur_left > 0) {
		dur_left -= 10;
		return 0;
	}
	else
		return 1;
}
 
 //Make a beep
 void beep(uint freq, uint dur) {

	//preemptive return
	if(freq == 0  && dur == 0) {
		nosound();
		return;
	}

    dur_left = dur;
    play_sound(freq);

	// cprintf has a side-effect we need.
	// without it, while loop never stops

	//changed from dur_left != 0 to >= 10:
	// if duration is less than 10ms, no sound can be produced (need >= 10ms)
    while (dur_left >= 10){cprintf("");/*do nothing*/};
    nosound();

 	//timer_wait(dur);
          //set_PIT_2(old_frequency);
 }


static struct sndpkt* sndbuf[max_length];
static int head = 0;

/* Appends sound pkts to buffer from start index to end index (end index excluded).
Returns 1 if all sound pkts have been added before buffer gets full.
Returns 0 if buffer is full but there are still more pkts to be played.
*/
int append_to_buf(int start, int end, struct sndpkt *pkts) {

    for (int i = start; i < end; i++) {
    	sndbuf[i]=pkts;

        if (pkts->duration == 0 && pkts->frequency == 0) {
            return 1;
        }

        if (head == max_length)  {
            //loop around to the beginning
            head = 0;
        }
        head++;
        pkts += 1;
    }

    //at this point, buffer is full but there are more pkts to be played
    return 0;
}


/* Plays the sound pkts from the buffer from index i (start) to index j (end).
If all pkts have been played, return 1.
Otherwise, once buffer is half drained return 0.
*/
int play_from_buf(struct sndpkt *sndbuf[], int i, int j) {
    int consumed = 0; //num of pkts consumed so far
    struct sndpkt *curr_pkt = sndbuf[i];
    int curr_pkt_freq = curr_pkt->frequency;
    int curr_pkt_dur = curr_pkt->duration;

    while (consumed != max_length/2) {
    	int count_ms = 0;

    	if (curr_pkt_freq == 0 && curr_pkt_dur == 0)
    		return 1;

    	if (curr_pkt_dur >= 10) {
    		beep(curr_pkt_freq, curr_pkt_dur);
    		consumed++;
    		i++;
    		curr_pkt = sndbuf[i];
    		curr_pkt_freq = curr_pkt->frequency;
    		curr_pkt_dur = curr_pkt->duration;
    	}
    	else {
    		//play the note
    		beep(curr_pkt_freq, 10);

    		//get the next pkt
    		count_ms += curr_pkt_dur;
    		for (int k = i+1; k < j; k++) {

    			count_ms += sndbuf[k]->duration;
    			if (count_ms == 10) {
    				//lands exactly at beginning of next packet
    				i=k+1;
    				curr_pkt = sndbuf[i];
    				curr_pkt_freq = curr_pkt->frequency;
    				curr_pkt_dur = curr_pkt->duration;
    				break;

    			}
    			if (count_ms > 10) {
    				//lands in the middle of a packet
    				i=k;
    				curr_pkt = sndbuf[k];
    				curr_pkt_freq = curr_pkt->frequency;
    				curr_pkt_dur = count_ms-10;
    				break;
    			}
    			consumed++;
    		}

    	}
	
    }
    return 0;

}


void play(struct sndpkt *pkts){
	//proper way is to append pkts to a buffer
	//then play from said buffer

	int buf_flag, play_flag;
	int first_pass=1;
	int buf1 = 1; //first half of buf
	int buf2 = 0; //second half of buf

	while (1) {

		if (first_pass) {
			first_pass=0;

			//SPINLOCK
			acquire(&buflock);
			buf_flag = append_to_buf(0, max_length, pkts);
			pkts += max_length; //increment pointer, since it's pass by value
			release(&buflock);

			if (!buf_flag) {
				//SLEEP
				//block process trying to append packets as the buf is now full
			}

			play_flag = play_from_buf(sndbuf, 0, max_length);
			if (!play_flag) {
				//enough packets have been consumed
				//WAKEUP process waiting for space in buf
			}
			else {
				//otherwise, all sound pkts are done playing
				cprintf("DONE!");
				break;
			}

		}
		
		if (buf1) {
			//first half of buf has already been played/buf drained 50%.
			//for process waiting for space in buf, can now append more packets to this first half

			buf1=0;
			buf2=1;
			//SPINLOCK 
			acquire(&buflock);
			buf_flag = append_to_buf(0, max_length/2,pkts);
			pkts += max_length/2;
			release(&buflock);

			if (!buf_flag) {
				//SLEEP
				//block process trying to append packets as the buf is now full
			}

			play_flag = play_from_buf(sndbuf, max_length/2, max_length);
			if (!play_flag) {
				//enough packets have been consumed
				//WAKEUP process waiting for space in buf
			}
			else {
				//otherwise, all sound pkts are done playing
				cprintf("DONE!");
				break;
			}
		}

		if (buf2) {
			//seond half of buf has already been played/buf drained 50%.
			//for process waiting for space in buf, can now append more packets to this second half

			buf2=0;
			buf1=1;

			//SPINLOCK 
			acquire(&buflock);
			buf_flag = append_to_buf(max_length/2, max_length,pkts);
			pkts += max_length/2;
			release(&buflock);

			if (!buf_flag) {
				//SLEEP
				//block process trying to append packets as the buf is now full
			}

			play_flag = play_from_buf(sndbuf, 0, max_length/2);
			if (!play_flag) {
				//enough packets have been consumed
				//WAKEUP process waiting for space in buf
			}
			else {
				//otherwise, all sound pkts are done playing
				cprintf("DONE!");
				break;
			}

		}
	}
	
	return; 
}