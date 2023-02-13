#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "sound.h"

int dur_left;

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
 
 //make it shutup. Comment this back in soon
 static void nosound() {
 	uchar tmp = inb(0x61) & 0xFC;
 
 	outb(0x61, tmp);
 }

 int beep_timer() {
    if (dur_left) {
        dur_left--;
        return 0;
    }
    else
        return 1;
 }
 
 //Make a beep
 void beep(int freq, int dur) {
    dur_left = dur;
    play_sound(freq);
    while (!beep_timer());
    nosound();


 	//timer_wait(dur);
 	//nosound();
          //set_PIT_2(old_frequency);
 }
