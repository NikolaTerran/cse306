CSE306 HW4

Exercise 1:
Transmission of characters are now driven by interrupts, rather than a polling loop.

There is a conditional compilation using #ifdef/#endif for debugging printout, in line 15 in the uart.c file. To turn it off, type "#undef DEBUG_PRINT". To turn it on, type "#define DEBUG_PRINT 1". The debugging printout traces calls to uartputc(), sleep(), wakeup() (which occurs every time there is free space in the output buffer), as well as the occurrence of transmitter and receiver interrupts. Note that once debugging printout is turned on, the characters being outputted to the terminal becomes quite slow, since cprintf() slows it down.


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


To enable DMA transfers, in line 28 of ide.c, set "#define DMA" to be 1. To enable PIO mode transfers, set "#define DMA" to be 0.

Getting DMA to work in Bochs:

It was challenging getting DMA to work in Bochs, mainly because the PCI configuration in QEMU was not compatible with Bochs. For example, under QEMU, the PCI IDE controller identifies as vendor ID 0x8086, device ID 0x7010. These were found in function 1 of the 82371 chip in PCI bus 0, slot 1. This is not the case with Bochs, since its PCI configuration space is different.