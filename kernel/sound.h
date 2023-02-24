struct sndpkt {
	uint frequency; //frequency in millihertz
	uint duration;	//duration in milliseconds
};


//keep it small to test blocking
#define max_length 12
//buffer defined in sound.c


void beep(uint, uint);
int beep_timer();
void play(struct sndpkt *pkts);
