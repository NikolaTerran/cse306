// Simple PIO-based (non-DMA) IDE driver code.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "buf.h"

#define SECTOR_SIZE   512
#define IDE_BSY       0x80
#define IDE_DRDY      0x40
#define IDE_DF        0x20
#define IDE_ERR       0x01

#define IDE_CMD_READ  0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_RDMUL 0xc4
#define IDE_CMD_WRMUL 0xc5



// A PRDT consists of one or more Physical Region Descriptors (PRDs), 
// each of which describes the source or destination in memory for a transfer operation to be performed by the disk drive. 
// Each PRD is an eight-byte entry: the first four bytes are the physical memory address at which data is to be read or written; 
// the next two bytes give the number of bytes to be transferred (value 0 means the maximum of 64K bytes). 
// The two bytes are reserved, except for the most-significant bit (called "end of transfer", or EOT), 
// which is set to indicate to the DMA controller that that particular PRD is the last entry in the PRDT.

#define PRDT_ENTRIES 1

struct prd {
    uint address;
    ushort bytes;
    ushort eot;
};

struct prd __attribute__ ((aligned (4))) prd_table[PRDT_ENTRIES];

void init_prdt_table(void) {
    int i;
    for (i = 0; i < PRDT_ENTRIES; i++) {
        prd_table[i].address = 0;
        prd_table[i].bytes = 0;
        prd_table[i].eot = 0;
    }
    prd_table[i].eot = 0x80;
}

// port for bus master register
static uint dma_port;

void checkFunction(char bus, char device, char function);

// idequeue points to the buf now being read/written to the disk.
// idequeue->qnext points to the next buf to be processed.
// You must hold idelock while manipulating queue.

static struct spinlock idelock;
static struct buf *idequeue;

static int havedisk1;
static void idestart(struct buf*);

// Wait for IDE disk to become ready.
static int
idewait(int checkerr)
{
  int r;

  while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
    ;
  if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
    return -1;
  return 0;
}

//get word instead of byte
static inline ushort
inw(ushort port)
{
  ushort data;

  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
  return data;
}

//get u32int
static inline unsigned long
inl(ushort port)
{
  unsigned long data;

  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
  return data;
}

//output u32int
static inline void
outl(ushort port, unsigned long data)
{
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

char getBaseClass(char bus, char device, char function){
  outl(0xcf8, 0x80000000 | (bus<<16) | (device<<11) | (function<<8) | 0xB);
  return inb(0xcfc);
}

char getSubClass(char bus, char device, char function){
  outl(0xcf8, 0x80000000 | (bus<<16) | (device<<11) | (function<<8) | 0xA);
  return inb(0xcfc);
}

char getSecondaryBus(char bus, char device, char function){
  outl(0xcf8, 0x80000000 | (bus<<16) | (device<<11) | (function<<8) | 0x19);
  return inb(0xcfc);
}


//PCI Bus Device Enumeration #paragraph 2-1
//enumerate the devices that are connected to the bus, 
//identify their type, 
//and locate their interface registers in either the I/O port space or mapped into the physical address space. 
//assume that there is just one PCI bus and avoid complication
char getHeaderType(char bus, char device, char function) {
  outl(0xcf8, 0x80000000 | (bus<<16) | (device<<11) | (function<<8) | 0xE);
  return inb(0xcfc);
}

short getVendorID(char bus, char device, char function){
  outl(0xcf8, 0x80000000 | (bus<<16) | (device<<11) | (function<<8) | 0);
  return inw(0xcfc);
}

//PCI Bus Device Enumeration #paragraph 2-2
//Individual devices may have up to 8 separate "functions", each of which is uniquely identified by a number in the range [0, 7]. 
//For each slot, a command is sent to query the PCI configuration registers for function 0 of that device (every device is required to implement function 0, 
//  but not necessarily other functions) in order to read the "header type" byte from the configuration registers of that device. 
//If there is no device in the specified slot, a value of 0xFFFF will be returned. 
//Otherwise, the header type byte indicates the basic format of the configuration space of that device (header type 0x00 indicates a generic PCI device and header types 0x01 and 0x02 indicate bridges). 
//In addition, if bit 7 of the header type byte is set, then then the device is a multi-function device, so the other functions besides function 0 should also be queried. 
void checkDevice(char bus, char device) {
    char function = 0;
    short vendorID = getVendorID(bus, device, function);
    if (vendorID == 0xffff) return; // Device doesn't matter
    checkFunction(bus, device, function);
    int headerType = getHeaderType(bus, device, function);
    if( (headerType & 0x80) != 0) {
        // It's a multi-function device, so check remaining functions
        for (function = 1; function < 8; function++) {
            if (getVendorID(bus, device, function) != 0xFFFF) {
                checkFunction(bus, device, function);
            }
        }
    }
}

void checkBus(char bus) {
    char device;
    for (device = 0; device < 32; device++) {
        checkDevice(bus, device);
    }
}

//PCI Bus Device Enumeration #paragraph 2-3
//Once the presence of a device in a particular slot on a particular bus has been identified and the header type for that device determined, 
//each of the functions of that device is queried to read a 16-bit device ID, a 16-bit vendor ID, a one-byte class code, and a one-byte subclass code, from the device configuration registers. 
//The class code and subclass code identify the type of device. For example, class 0x1 and subclass 0x1 indicate an IDE controller, which is what we are interested in. 
//The vendor ID and the device ID provide additional identifying information about the device. Vendor ID 0x8086 is used by Intel. 
//Under QEMU, the PCI IDE controller identifies as vendor ID 0x8086, device ID 0x7010. This will be found as function 1 of the 82371 chip in PCI bus 0, slot 1. 
//The IDE controller itself will be operated in "compatibility mode", in which commands to perform disk operations are issued as if it were a legacy device on the ISA bus. 
//This is the way that the stock IDE driver in xv6 expects to interact with the IDE controller. The IDE controller works in conjunction with the Bus Master DMA controller, which is also part of the 82371 chip. 
void checkFunction(char bus, char device, char function) {

  ushort id = getVendorID(bus, device, function);
  if(id == 0x8086){
    //get device id
    outl(0xcf8, 0x80000000 | (bus<<16) | (device<<11) | (function<<8) | 0x2);
    if(inw(0xcfc) == 0x7010){
      //In order to engage DMA mode for IDE disk transfers, the DMA controller first has to be enabled. 
      //This is done by reading the command register at address 0x4 in the PCI configuration space for the Bus Master, 
      //setting bit 0 (value 0x1) and writing the updated value back to the command register. 
      //If you don't do this, then you won't ever get any interrupts back when you send a disk drive a "Read DMA" or "Write DMA" command.
      // The need for enabling the device in this way was not evident to me from the documents that I read, and it took quite awhile before I discovered it. 
      
      cprintf("vendor id: 0x%x    device id: 0x%x\n",id,inw(0xcfc));
      cprintf("ide controller found! setup control bit and store base address of I/O port here!\n");
      outl(0xcf8, 0x80000000 | (bus<<16) | (device<<11) | (function<<8) | 0x4);
      
      //the value is 103 -> 100000011
      //https://wiki.osdev.org/PCI#Command_Register
      cprintf("command reg: 0x%x\n",inw(0xcfc));
      
      outl(0xcf8, 0x80000000 | (bus<<16) | (device<<11) | (function<<8) | 0x20);
      dma_port = inl(0xcfc) & 0xfffffff0;
      cprintf("io_port: 0x%x\n",dma_port);
      //https://wiki.osdev.org/ATA/ATAPI_using_DMA
    }
  }

  char baseClass;
  char subClass;
  char secondaryBus;

  baseClass = getBaseClass(bus, device, function);
  subClass = getSubClass(bus, device, function);
  if ((baseClass == 0x6) && (subClass == 0x4)) {
      secondaryBus = getSecondaryBus(bus, device, function);
      checkBus(secondaryBus);
  }  
}


//There can be up to 256 PCI buses on a system. Each bus is uniquely identified by a number in the range [0, 255]. 
//Each bus has up to 32 "slots" into which devices are connected. 
//Each slot is uniquely identified by a number in the range [0, 31]. 
//For each bus, the slots are scanned, also in increasing numerical order. 

void checkAllBuses(void) {
 
    // // debug, it shows the config of the first pci device
    // int i = 0;
    // while(i < 16){
    //   outl(0xcf8, 0x80000000 | (0<<16) | (1<<11) | (0<<8) | i);
    //   cprintf(" %x ",inw(0xcfc));
    //   i += 2;
    // }

     char function;
     char bus;

     short headerType = getHeaderType(0, 0, 0);
     if ((headerType & 0x80) == 0) {
         // Single PCI host controller
         checkBus(0);
     } else {
         // Multiple PCI host controllers
         for (function = 0; function < 8; function++) {
             if (getVendorID(0, 0, function) != 0xFFFF) break;
             bus = function;
             checkBus(bus);
         }
     }
 }

//PCI Bus Device Enumeration #paragraph 4 and 5
//In order to engage DMA mode for IDE disk transfers, the DMA controller first has to be enabled. 
//This is done by reading the command register at address 0x4 in the PCI configuration space for the Bus Master, 
//setting bit 0 (value 0x1) and writing the updated value back to the command register. 

//The next thing you have to do in order to be able to use the DMA controller is to find out the base address of its I/O ports. 
//In general, these will either be located in I/O port space (accessed by inb, outb, etc.) or mapped into the physical address space, where they are accessed like ordinary memory. 
//As it turns out, the BIOS under QEMU and Bochs initializes the DMA controller so that its ports are located in I/O port space. 
//The way you find out the base address of the I/O ports is by reading BAR4 (Base Address Register 4), which is a 32-bit field of the PCI configuration space for the device. 
//The address you obtain in this way will always be four-byte aligned, but Bit 0 (value 0x1) is used to indicate whether the address is in I/O port space (bit 0 == 0x1) or memory mapped (bit 0 == 0x0). 
//You must mask out (i.e. set to zero) the least significant four bits of the value read from BAR4 to get the correct base address. 
void
ideinit(void)
{
  
  initlock(&idelock, "ide");
  ioapicenable(IRQ_IDE, ncpu - 1);

  checkAllBuses();
  init_prdt_table();
  cprintf("ideinit complete\n");
  // Old code
  int i;
  idewait(0);
  // Check if disk 1 is present
  outb(0x1f6, 0xe0 | (1<<4));
  for(i=0; i<1000; i++){
    if(inb(0x1f7) != 0){
      havedisk1 = 1;
      break;
    }
  }
  
  // Switch back to disk 0.
  outb(0x1f6, 0xe0 | (0<<4));
  // End old code
}



// Start the request for b.  Caller must hold idelock.
static void
idestart(struct buf *b)
{
  if(b == 0)
    panic("idestart");
  if(b->blockno >= FSSIZE)
    panic("incorrect blockno");
  int sector_per_block =  BSIZE/SECTOR_SIZE;
  int sector = b->blockno * sector_per_block;
  int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
  int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;

  if (sector_per_block > 7) panic("idestart");

  idewait(0);
  // load first half of parameters?
  outb(0x3f6, 0);  // generate interrupt
  outb(0x1f2, sector_per_block);  // number of sectors
  outb(0x1f3, sector & 0xff);
  outb(0x1f4, (sector >> 8) & 0xff);
  outb(0x1f5, (sector >> 16) & 0xff);
  outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
  if(b->flags & B_DIRTY){
    outb(0x1f7, write_cmd);
    outsl(0x1f0, b->data, BSIZE/4);
  } else {
    outb(0x1f7, read_cmd);
  }
}

// Interrupt handler.
void
ideintr(void)
{
  struct buf *b;

  // First queued buffer is the active request.
  acquire(&idelock);

  if((b = idequeue) == 0){
    release(&idelock);
    return;
  }
  idequeue = b->qnext;

  // Read data if needed.
  if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
    insl(0x1f0, b->data, BSIZE/4);

  // Wake process waiting for this buf.
  b->flags |= B_VALID;
  b->flags &= ~B_DIRTY;
  wakeup(b);

  // Start disk on next buf in queue.
  if(idequeue != 0)
    idestart(idequeue);

  release(&idelock);
}

//PAGEBREAK!
// Sync buf with disk.
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
  struct buf **pp;

  if(!holdingsleep(&b->lock))
    panic("iderw: buf not locked");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("iderw: nothing to do");
  if(b->dev != 0 && !havedisk1)
    panic("iderw: ide disk 1 not present");

  acquire(&idelock);  //DOC:acquire-lock

  // Append b to idequeue.
  b->qnext = 0;
  for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
    ;
  *pp = b;

  // Start disk if necessary.
  if(idequeue == b)
    idestart(b);

  // Wait for request to finish.
  while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
    sleep(b, &idelock);
  }


  release(&idelock);
}
