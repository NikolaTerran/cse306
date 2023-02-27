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


static struct sndpkt sndbuf[max_length];

/* Appends sound pkts to buffer from start index to end index (end index excluded).
Returns 1 if all sound pkts have been added before buffer gets full.
Returns 0 if buffer is full but there are still more pkts to be played.
*/
int buf_head = 0;
int pkt_inc = 0;
int free_space = max_length;

struct sndpkt * append_to_buf(struct sndpkt *pkts) {
	// cprintf("begin free_space : %d\n",free_space);

	while(free_space != 0){
		if(buf_head == max_length){
			buf_head = 0;
		}

		if (pkts->duration == 0 && pkts->frequency == 0) {
			//make it zero
			sndbuf[buf_head].frequency = 0;
			sndbuf[buf_head].duration = 0;
			// cprintf("return from append_to_buf:\n");
			// cprintf("sndbuf[0]: %d\n",sndbuf[0].frequency);
			// cprintf("sndbuf[1]: %d\n",sndbuf[1].frequency);
			// cprintf("sndbuf[2]: %d\n",sndbuf[2].frequency);
			// cprintf("sndbuf[3]: %d\n",sndbuf[3].frequency);
			// cprintf("end free_space : %d\n",free_space);
            return 0;
        }

		// cprintf("buf_head: %d, frequency: %d\n",buf_head,pkts->frequency);
		sndbuf[buf_head].frequency=pkts->frequency;
		sndbuf[buf_head].duration=pkts->duration;
		buf_head++;
		free_space--; //May need a lock for free_space?
        pkts++;
		pkt_inc++;
	}
    //at this point, buffer is full but there are more pkts to be played
	// cprintf("return from append_to_buf:\n");
	// cprintf("sndbuf[0]: %d\n",sndbuf[0].frequency);
	// cprintf("sndbuf[1]: %d\n",sndbuf[1].frequency);
	// cprintf("sndbuf[2]: %d\n",sndbuf[2].frequency);
	// cprintf("sndbuf[3]: %d\n",sndbuf[3].frequency);
	// cprintf("end free_space : %d\n",free_space);
    return pkts;
}

/* Plays the sound pkts from the buffer from index i (start) to index j (end).
If all pkts have been played, return 1.
Otherwise, once buffer is half drained return 0.
*/
int play_head = 0;
int isplaying = 0;

int play_from_buf() {

	int curr_pkt_freq = sndbuf[play_head].frequency;
    int curr_pkt_dur = sndbuf[play_head].duration;

    while (1) {
    	// int count_ms = 0;

		if(play_head == max_length){
			play_head = 0;
		}

    	if (curr_pkt_freq == 0 && curr_pkt_dur == 0){
			isplaying = 0;
			// cprintf("return from play_from_buf\n");
			return 1;
		}
		else if (curr_pkt_dur >= 10) {
			// cprintf("play head: %d || frequency: %d\n, dur: %d\n", play_head,curr_pkt_freq, curr_pkt_dur);
    		beep(curr_pkt_freq, curr_pkt_dur);
			
			//free the buffer
			sndbuf[play_head].frequency=0;
    		sndbuf[play_head].duration=0;

    		free_space++;
			// cprintf("free_space after play: %d\n",free_space);
    		play_head++;
    		curr_pkt_freq = sndbuf[play_head].frequency;
    		curr_pkt_dur = sndbuf[play_head].duration;

    		if (free_space >= max_length/2) {
				//enough packets have been consumed
				//WAKEUP process waiting for space in buf
				// cprintf("Waking up proc\n");
				wakeup(&free_space);
				
			}
    	}

    	else{
			//duration less than 10
    		int count_ms=0;

    		if (sndbuf[play_head+1].frequency==0 && sndbuf[play_head+1].duration==0){
    			break;
    		}

    		// cprintf("Playing pkt freq: %d, dur: %d\n", curr_pkt_freq, 10);
    		beep(curr_pkt_freq,10);
    		//get the next pkt
    		count_ms += curr_pkt_dur;
    		for (int k = play_head+1; k < max_length; k++) {

    			if (sndbuf[k].frequency==0 && sndbuf[k].duration==0){
    				break;
    			}

    			count_ms += sndbuf[k].duration;

    			if (count_ms == 10) {
    				//lands exactly at beginning of next packet

    				//free the current pkt at index k
    				// cprintf("Discarding pkt freq: %d, dur: %d\n", sndbuf[k].frequency, sndbuf[k].duration);
    		 		sndbuf[k].frequency=0;
    				sndbuf[k].duration=0;
    				free_space++;

    		 		play_head=k+1;
    		 		curr_pkt_freq = sndbuf[play_head].frequency;
    				curr_pkt_dur = sndbuf[play_head].duration;
    		 		break;
    			}
    			if (count_ms > 10) {
    		 		//lands in the middle of a packet
    		 		play_head=k;
    		 		curr_pkt_freq = sndbuf[play_head].frequency;
    		 		curr_pkt_dur = count_ms-10;
    		 		break;
    		 	}
    		 	// cprintf("Discarding pkt freq: %d, dur: %d\n", sndbuf[k].frequency, sndbuf[k].duration);
    		 	//free the current pkt at index k
    		 	sndbuf[k].frequency=0;
    			sndbuf[k].duration=0;
    		 	free_space++;

    		 	if (free_space >= max_length/2) {
					//enough packets have been consumed
					//WAKEUP process waiting for space in buf
					// cprintf("Waking up proc\n");
					wakeup(&free_space);
				}
    		}

    	}
    }
    return 0;
}

void play(struct sndpkt *pkts){
	//proper way is to append pkts to a buffer
	//then play from said buffer
	// int buf_flag;
	while (1) {
		//SPINLOCK 
		acquire(&buflock);
		// cprintf("called playing\n");
		if (free_space == 0) { //buffer currently full
			// cprintf("Buffer full, sleeping on chan\n");
			sleep(&free_space, &buflock);
		}
		pkts = append_to_buf(pkts);
		// pkt_inc = 0;
		release(&buflock);

		if(!pkts){
			// cprintf("All packets appended!\n");
			releasesleep(&playlock);
		}

		if(isplaying){
			// donothing
		}else{
			isplaying = 1;
			play_from_buf();
		}
		
		if(!pkts){
			break;
		}
	}
	
	return; 
}