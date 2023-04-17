


Exercise 2:
PCI Bus Device Enumeration

So basically we want to loop over all 65535 i/o port
port value is calculated by bus << 8 + device << 3 + function (note: use Configuration Access Mechanism to read the configuration instead of read from the port directly)
read a word off of port gives us the vendor id
we want to find a device whose vendor id = 8086 and device id of 0x7010
then we can find BAR4 based on that:
https://stackoverflow.com/questions/30190050/what-is-the-base-address-register-bar-in-pcie

To get the specific pci configuration in the table, you have to write to (0x80000000 | bus << 16 | device << 11 | function <<  8 | offset) with outb
The offset range from [0-64] which corresponds to the 64 bytes in the standard pci config
then read from (0xcfc) with inb
also see the "software implementation" part of 
https://en.wikipedia.org/wiki/PCI_configuration_space




Using DMA controller:

The control flow almost exactly mimics the code from this post:
https://forum.osdev.org/viewtopic.php?f=1&t=30498
Some wait operations are just being replaced by the default idewait.
There is so much going on with this procedure, the plain description
from osdev articles are not suffice for me to implement it without 
messing something up in the sequence. Following this working example
is the only way I'm getting out of this debuggin hell.

2 major hurdles we encountered in this section:
1. not getting the interrupt from the device
2. the disk data is not written into the buffer (and vice versa)

the second hurdle is mainly solved by adding additional idewait operations
here and there. The first one is probably some address error that
I stuck for hours with.



physical region descriptor table:

//from bing ai
```Here is an example of how to generate a PRDT in C :
struct prdt {
    uint32_t address;
    uint32_t length;
};

struct prdt prdt_table[PRDT_ENTRIES];

void init_prdt_table(void) {
    int i;
    for (i = 0; i < PRDT_ENTRIES; i++) {
        prdt_table[i].address = 0;
        prdt_table[i].length = 0;
    }
}```

I modified it so that length is divided into 2 ushort field.
and eot is assigned a value of 0x8000.
I also attempted multiple ways of making PRDT since:
1. I'm not getting interrupts from the device.
2. the result is not written into the buffer.
Turns out there's nothing wrong with ai's code template
It's just something I did wrong in the previous section.


benchmark.c runtime (timed by hand):

DMA :-
write: (5.15 + 4.8 + 5.28 + 5.03 + 5.38)/5 = 5.128 seconds
read: (4.41 + 4.5 + 4.36 + 4.42 + 4.96)/5 = 4.53 seconds

PIO :- 
write: (5.40 + 4.98 + 4.89 + 4.9 + 5.37)/5 = 5.108 seconds
read: (3.93 + 4.55 + 4.47 + 4.5 + 5.0)/5 = 4.49 seconds

The result is consistent with what I have seen on OSDEV.org, where PIO is slightly
faster than DMA utilizing single PRD entry.


A note about bochs:

It doesn't work with the way we address the PCI config space,
when we check the offset 0x2 in the space it is not device id
but another entry of vendor id. Either there is something is wrong with
the bochs I install on my computer or bochs is using another way
to address the pci configuration space, which we don't have enough
time to investigate.