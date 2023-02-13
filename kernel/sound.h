struct sndpkt {
    uint frequency;   // frequency in millihertz
    uint duration;    // duration in milliseconds
};

void beep(int, int);
int play(struct sndpkt *pkts);
