


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


physical region descriptor table:
