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
struct spinlock sleepwakelock;

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

/* Appends sound pkts to buffer from start index to end index (end index excluded).
Returns 1 if all sound pkts have been added before buffer gets full.
Returns 0 if buffer is full but there are still more pkts to be played.
*/
int buf_head = 0;
int free_space = max_length;

int append_to_buf(struct sndpkt *pkts) {

	cprintf("before:\n");
	cprintf("pointer sndbuf: %p\n",sndbuf[0]);
	cprintf("sndbuf[0]: %d\n",sndbuf[0]->frequency);
	cprintf("sndbuf[1]: %d\n",sndbuf[1]->frequency);
	cprintf("sndbuf[2]: %d\n",sndbuf[2]->frequency);
	cprintf("sndbuf[3]: %d\n",sndbuf[3]->frequency);
	cprintf("sndbuf[4]: %d\n",sndbuf[4]->frequency);
	cprintf("sndbuf[5]: %d\n",sndbuf[5]->frequency);
	cprintf("sndbuf[6]: %d\n",sndbuf[6]->frequency);
	cprintf("sndbuf[7]: %d\n",sndbuf[7]->frequency);
	cprintf("sndbuf[8]: %d\n",sndbuf[8]->frequency);
	cprintf("sndbuf[9]: %d\n",sndbuf[9]->frequency);

	if(buf_head == max_length){
		buf_head = 0;
	}

    for (int i = buf_head; i < max_length; i++) {
    
        if (pkts->duration == 0 && pkts->frequency == 0) {
			//make it zero
			sndbuf[buf_head]->frequency = 0;
			sndbuf[buf_head]->duration = 0;
			buf_head++;
			free_space--;
			cprintf("after:\n");
			cprintf("pointer sndbuf: %p\n",sndbuf);
			cprintf("pointer sndbuf[0]: %p\n",sndbuf[0]);
			cprintf("sndbuf[0]: %d\n",sndbuf[0]->frequency);
			cprintf("sndbuf[1]: %d\n",sndbuf[1]->frequency);
			cprintf("sndbuf[2]: %d\n",sndbuf[2]->frequency);
			cprintf("sndbuf[3]: %d\n",sndbuf[3]->frequency);
			cprintf("sndbuf[4]: %d\n",sndbuf[4]->frequency);
			cprintf("sndbuf[5]: %d\n",sndbuf[5]->frequency);
			cprintf("sndbuf[6]: %d\n",sndbuf[6]->frequency);
			cprintf("sndbuf[7]: %d\n",sndbuf[7]->frequency);
			cprintf("sndbuf[8]: %d\n",sndbuf[8]->frequency);
			cprintf("sndbuf[9]: %d\n",sndbuf[9]->frequency);
            return 1;
        }

		sndbuf[buf_head]=pkts;
		cprintf("buf_head: %d\n",buf_head);
		cprintf("frequency: %d\n",pkts->frequency);
		buf_head++;
		free_space--; //May need a lock for free_space?
        pkts += 1;
    }
    //at this point, buffer is full but there are more pkts to be played
    return 0;
}

/* Plays the sound pkts from the buffer from index i (start) to index j (end).
If all pkts have been played, return 1.
Otherwise, once buffer is half drained return 0.
*/
int play_head = 0;
int isplaying = 0;

int play_from_buf() {
    struct sndpkt *curr_pkt = sndbuf[play_head];
    int curr_pkt_freq = curr_pkt->frequency;
    int curr_pkt_dur = curr_pkt->duration;

    while (1) {
    	int count_ms = 0;

		if(play_head == max_length){
			play_head = 0;
		}

    	if (sndbuf[play_head]->frequency == 0 && sndbuf[play_head]->duration == 0){
			isplaying = 0;
			cprintf("play_head: %d\n",play_head);
			cprintf("freq: %d\n",sndbuf[play_head]->frequency);
			return 1;
		}

    	else if (curr_pkt_dur >= 10) {
			cprintf("play head: %d\n", play_head);
			cprintf("frequency: %d\n", sndbuf[play_head]->frequency);
    		beep(sndbuf[play_head]->frequency, sndbuf[play_head]->duration);
			//free the buffer

			//sndbuf[play_head]->frequency=0;
    		//sndbuf[play_head]->duration=0;

    		free_space++;
    		play_head++;
    	}
    	else{
			//please change else accrodingly
			cprintf("not called\n");
    		//play the note
			cprintf("play head: %d\n", play_head);
			cprintf("playing frequency: %d\n",curr_pkt_freq);
    		beep(curr_pkt_freq, 10);
			//free the buffer
			// curr_pkt->frequency=0;
    		// curr_pkt->duration=0;

    		//get the next pkt
    		count_ms += curr_pkt_dur;
    		for (int k = play_head+1; k < max_length; k++) {

    			count_ms += sndbuf[k]->duration;
    			if (count_ms == 10) {
    				//lands exactly at beginning of next packet
    				play_head=k+1;
    				curr_pkt = sndbuf[play_head];
    				curr_pkt_freq = curr_pkt->frequency;
    				curr_pkt_dur = curr_pkt->duration;
    				break;

    			}
    			if (count_ms > 10) {
    				//lands in the middle of a packet
    				play_head=k;
    				curr_pkt = sndbuf[k];
    				curr_pkt_freq = curr_pkt->frequency;
    				curr_pkt_dur = count_ms-10;
    				break;
    			}
    			free_space++;
    		}
    	}
    }
    return 0;
}

void play(struct sndpkt *pkts){
	//proper way is to append pkts to a buffer
	//then play from said buffer

	acquiresleep(&playlock);
	int buf_flag;

	while (1) {
		//SPINLOCK 
		acquire(&buflock);

		while (free_space == 0) { //buffer currently full
			cprintf("Buffer full, sleeping on chan\n");
			sleep(&free_space, &buflock);
		}
		buf_flag = append_to_buf(pkts);
		pkts += max_length;
		release(&buflock);

		cprintf("All packets appended!\n");
		releasesleep(&playlock);

		/* if (!buf_flag) {
			//SLEEP
			//block process trying to append packets as the buf is now full
		}else{
			cprintf("All packets appended!\n");
			releasesleep(&playlock);
		} */

		// if(isplaying){
		// 	//donothing
		// 	cprintf("something is playing: %d\n",isplaying);
		// 	play_from_buf();
		// }else{
		// 	isplaying = 1;
		// 	play_from_buf();
		// }
		/* if(sndbuf[0]->frequency == 1000){
				play_from_buf();
			} */

		play_from_buf();

		if (free_space >= max_length/2) {
			//enough packets have been consumed
			//WAKEUP process waiting for space in buf
			cprintf("Waking up proc\n");
			wakeup(&free_space);
		}
		
		if(buf_flag){
			//otherwise, all sound pkts are done playing
			cprintf("DONE!");
			break;
		}

		// if (buf1) {
		// 	//first half of buf has already been played/buf drained 50%.
		// 	//for process waiting for space in buf, can now append more packets to this first half

		// 	buf1=0;
		// 	buf2=1;
		// 	//SPINLOCK 
		// 	acquire(&buflock);
		// 	buf_flag = append_to_buf(0, max_length/2,pkts);
		// 	pkts += max_length/2;
		// 	release(&buflock);

		// 	if (!buf_flag) {
		// 		//SLEEP
		// 		//block process trying to append packets as the buf is now full
		// 	}else{
		// 		cprintf("All packets appended!\n");
		// 		releasesleep(&playlock);
		// 	}

		// 	play_flag = play_from_buf(sndbuf, 0, max_length/2);
		// 	if (!play_flag) {
		// 		//enough packets have been consumed
		// 		//WAKEUP process waiting for space in buf
		// 	}
		// 	else {
		// 		//otherwise, all sound pkts are done playing
		// 		cprintf("DONE!");
		// 		break;
		// 	}
		// 	return;
		// }

		// if (buf2) {
		// 	//seond half of buf has already been played/buf drained 50%.
		// 	//for process waiting for uspace in buf, can now append more packets to this second half

		// 	buf2=0;
		// 	buf1=1;

		// 	//SPINLOCK 
		// 	acquire(&buflock);
		// 	buf_flag = append_to_buf(max_length/2, max_length,pkts);
		// 	pkts += max_length/2;
		// 	release(&buflock);

		// 	if (!buf_flag) {
		// 		//SLEEP
		// 		//block process trying to append packets as the buf is now full
		// 	}else{
		// 		cprintf("All packets appended!\n");
		// 		releasesleep(&playlock);
		// 	}

		// 	play_flag = play_from_buf(sndbuf, max_length/2, max_length);
		// 	if (!play_flag) {
		// 		//enough packets have been consumed
		// 		//WAKEUP process waiting for space in buf
		// 	}
		// 	else {
		// 		//otherwise, all sound pkts are done playing
		// 		cprintf("DONE!");
		// 		break;
		// 	}

		// }
	}
	
	return; 
}