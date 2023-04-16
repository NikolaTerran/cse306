


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


//from bing bot 
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
}
This code initializes a PRDT table with zeroed out entries . You can modify this code to suit your needs.

I hope this helps! Let me know if you have any other questions.```

No sources were given. I'm not sure where did it rip the code from, but maybe the code'll work.

Also, add check if entries in prdt is within 64 kb boundary?