struct sndpkt {
	uint frequency; //frequency in millihertz
	uint duration;	//duration in milliseconds
}

void beep(uint, uint);
void play(struct sndpkt *pkts);
