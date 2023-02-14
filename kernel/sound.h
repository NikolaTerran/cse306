struct sndpkt {
    uint frequency;   // frequency in millihertz
    uint duration;    // duration in milliseconds
};

// //wait for implementation

// int max_length = 10;
// struct LinkedList {
//     struct sndpkt * packet;
//     struct LinkedList * next;
// }

// //or

// struct sndpkt packet[max_length];


void beep(int, int);
int play(struct sndpkt *pkts);
