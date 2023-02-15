#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "sound.h"


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
		// cprintf("dur_left: %d\n",dur_left);
		dur_left -= 10;
		return 0;
	}
	else
		return 1;
}
 
 //Make a beep
<<<<<<< HEAD
 void beep(uint freq, uint dur) {
=======
 void beep(int freq, int dur) {

	//preemptive return
	if(freq == 0  && dur == 0){
		nosound();
		return;
	}

>>>>>>> collab/main
    dur_left = dur;
    play_sound(freq);

	// cprintf has a side-effect we need
	// without it, while loop never stops
    while (dur_left != 0){cprintf("");/*do nothing*/};
    nosound();
 	//timer_wait(dur);
 	//nosound();
<<<<<<< HEAD
          //set_PIT_2(old_frequency);
 }

//Play music
void play(struct sndpkt *pkts) {

=======
	//set_PIT_2(old_frequency);
 }


static struct sndpkt* sndbuf[max_length];
static int head = 0;

static void play_helper(){
	beep(sndbuf[head]->frequency,sndbuf[head]->duration);
	//implement some way for helper to know beep has finished playing
	
	//take out the packet
	sndbuf[head] = 0;
}

int play(struct sndpkt *pkts){
	//proper way is to append pkts to a buffer
	//then play from said buffer
	
	

	//insertion
	if(sndbuf[head] == 0){
		sndbuf[head] = pkts;
	}else{
		//need implement some waiting
	}

	cprintf("head position: %d\n",head);
	play_helper();
	//helper needs to complete first
	
	head += 1;
	if(head == max_length){
		head = 0;
	}

	// beep(pkts->frequency,pkts->duration);
	return 0;
>>>>>>> collab/main
}