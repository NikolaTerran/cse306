struct sndpkt {
<<<<<<< HEAD
	uint frequency; //frequency in millihertz
	uint duration;	//duration in milliseconds
}

void beep(uint, uint);
void play(struct sndpkt *pkts);
=======
    uint frequency;   // frequency in millihertz
    uint duration;    // duration in milliseconds
};


//keep it small to test blocking
#define max_length 5
//buffer defined in sound.c


void beep(int, int);
int beep_timer();
int play(struct sndpkt *pkts);
>>>>>>> collab/main
