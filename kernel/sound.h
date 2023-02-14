struct sndpkt {
    uint frequency;   // frequency in millihertz
    uint duration;    // duration in milliseconds
};

// //wait for implementation
// struct LinkedList {
//     struct sndpkt * packet;
//     struct LinkedList * next;
// }

// //or

//keep it small to test blocking
#define max_length 5
//buffer defined in sound.c

void beep(int, int);
int play(struct sndpkt *pkts);
