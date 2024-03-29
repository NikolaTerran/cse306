// Intel 8250 serial port (UART).

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "ufs.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#undef DEBUG_PRINT


#define COM1_PORT    0x3f8
#define COM2_PORT    0x2f8

static int uart;    // is there a uart?

//lock for COM1 buffer
static struct {
  struct spinlock lock;
  int locking;
} uartlock1;

//lock for COM2 buffer
static struct {
  struct spinlock lock;
  int locking;
} uartlock2;

//lock for inbound_port
static struct {
  struct spinlock lock;
  int locking;
} portlock;

#define BACKSPACE 0x100
#define INPUT_BUF 128

static int com1_size = 0;
//buffer for accumulating input chars arriving on COM1
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} uart_input1;

static int com2_size = 0;
//buffer for accumulating input chars arriving on COM2
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} uart_input2;

#define C(x)  ((x)-'@')  // Control-x

static int inbound_port = 0;


// HW4
#define OUTPUT_BUF 128
//for COM1:
static char output_buf1[OUTPUT_BUF];
int head1 = 0;
int poll_head1=0;
int freespace1 = OUTPUT_BUF;
struct spinlock output_buflock1;
int busy1 = 0;

//for COM2:
static char output_buf2[OUTPUT_BUF];
int head2 = 0;
int poll_head2=0;
int freespace2 = OUTPUT_BUF;
struct spinlock output_buflock2;
int busy2 = 0;


// Function to check if the transmitter is busy.
// If not, removes the first character in the output buffer and issues it 
// for transmission, setting "busy" to 1 to indicate that the transmitter is busy. 
void uartstart() {

  if (inbound_port == COM1_PORT) {
    if (inb(COM1_PORT+5) & 0x20) {
      busy1=0;
    }

    if (!busy1) {
      // transmitter ready to accept a character

      // removes the first character in the output buffer and issues it for transmission,
      if (poll_head1 == OUTPUT_BUF)
        poll_head1 = 0;

      int c = output_buf1[poll_head1];
      // cprintf("Polling %s from buf\n", &c);
      output_buf1[poll_head1] = '\0'; //clear the spot in buf
      poll_head1++;
      freespace1++;
      if (freespace1 > 0) {
        #ifdef DEBUG_PRINT
        cprintf("Free space in buf\n");
        #endif
        wakeup(&freespace1);
      }

      if(c == '\b'){
        outb(inbound_port, '\b');
        outb(inbound_port, ' ');
        outb(inbound_port, '\b');
      }else{
        if(c == '\n'){
          outb(inbound_port, c);
          while(com1_size){
            outb(inbound_port, '\b');
            outb(inbound_port, ' ');
            outb(inbound_port, '\b');
            com1_size -= 1;
          }
        }else{
          com1_size += 1;
        }
        if(c != '\n'){
          if(c == 27){
            outb(inbound_port, 218);
          }else{
            outb(inbound_port,c);
          } 
        }
      }
      busy1=1;
    }
  }



  if (inbound_port == COM2_PORT) {
    if (inb(COM2_PORT+5) & 0x20) {
      busy2=0;
    }

    if (!busy2) {
      // transmitter ready to accept a character

      // removes the first character in the output buffer and issues it for transmission,
      if (poll_head2 == OUTPUT_BUF)
        poll_head2 = 0;

      int c = output_buf2[poll_head2];
      // cprintf("Polling %s from buf\n", &c);
      output_buf2[poll_head2] = '\0'; //clear the spot in buf
      poll_head2++;
      freespace2++;
      if (freespace2 > 0) {
        #ifdef DEBUG_PRINT
        cprintf("Free space in buf\n");
        #endif
        wakeup(&freespace2);
      }

      if(c == '\b'){
        outb(inbound_port, '\b');
        outb(inbound_port, ' ');
        outb(inbound_port, '\b');
      }else{
        if(c == '\n'){
          outb(inbound_port, c);
          while(com2_size){
            outb(inbound_port, '\b');
            outb(inbound_port, ' ');
            outb(inbound_port, '\b');
            com2_size -= 1;
          }
        }else{
          com2_size += 1;
        }
        
        if(c != '\n'){
          if(c == 27){
            outb(inbound_port, 218);
          }else{
            outb(inbound_port,c);
          } 
        }
      }
      busy2=1;
    }
  }



}

void
uartputc(int c)
{
  // int i;

  if(!uart)
    return;
  // On my test system, Bochs (running at 1M instr/sec)
  // requires between 500 and 1000 iterations to avoid
  // output overrun, as indicated by log messages in the
  // Bochs log file.  This is with microdelay() having an
  // empty body and ignoring its argument.
  // for(i = 0; i < 1000 && !(inb(COM1_PORT+5) & 0x20); i++)
  //  microdelay(10);

  // Puts the character in an output buffer, rather than polling
  // the device and outputting it directly. 

  #ifdef DEBUG_PRINT
  cprintf("Inside uartputc\n");
  #endif

  //If output buf is full, block process using sleep()
  if (inbound_port == COM1_PORT) {
    acquire(&output_buflock1);
    if (freespace1 == 0) { 
      //buffer currently full
      #ifdef DEBUG_PRINT
      cprintf("Buffer full, sleeping on chan\n");
      #endif
      sleep(&freespace1, &output_buflock1);
    }
    if (head1 == OUTPUT_BUF) {
      //loop around, circular buffer
      head1 = 0;
    }

    if (c == BACKSPACE)
      output_buf1[head1] = '\b';
    else
      output_buf1[head1] = c;

    head1++;
    freespace1--;
    release(&output_buflock1);
  }

  if (inbound_port == COM2_PORT) {
    acquire(&output_buflock2);
    if (freespace2 == 0) { 
      //buffer currently full
      #ifdef DEBUG_PRINT
      cprintf("Buffer full, sleeping on chan\n");
      #endif
      sleep(&freespace2, &output_buflock2);
    }
    if (head2 == OUTPUT_BUF) {
      //loop around, circular buffer
      head2 = 0;
    }

    if (c == BACKSPACE)
      output_buf2[head2] = '\b';
    else
      output_buf2[head2] = c;

    head2++;
    freespace2--;
    release(&output_buflock2);
  }

  uartstart();
  
}

static int
uartgetc(void)
{

  // cprintf("uartgetc:COM1 %d,COM2 %d\n", inb(COM1_PORT+5) & 0x01, inb(COM2_PORT+5) & 0x01 );
  
  //consider to lock it because of inbound_port change

  if(!uart)
    return -1;

  if(inb(COM1_PORT+5) & 0x01){
    acquire(&portlock.lock);
    inbound_port = COM1_PORT+0;
    release(&portlock.lock);
    return inb(COM1_PORT+0);
  }
    
  //copy-paste for com2
  if(inb(COM2_PORT+5) & 0x01){
    acquire(&portlock.lock);
    inbound_port = COM2_PORT+0;
    release(&portlock.lock);
    return inb(COM2_PORT+0);
  }

  return -1;
}

int
uartread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;
  iunlock(ip);
  target = n;

  if (ip->minor == 1) {

    acquire(&uartlock1.lock);
    while(n > 0){
      while(uart_input1.r == uart_input1.w){
        if(myproc()->killed){
          release(&uartlock1.lock);
          ilock(ip);
          return -1;
        }
        sleep(&uart_input1.r, &uartlock1.lock);
      }
      c = uart_input1.buf[uart_input1.r++ % INPUT_BUF];
      if(c == C('D')){  // EOF
        if(n < target){
          // Save ^D for next time, to make sure
          // caller gets a 0-byte result.
          uart_input1.r--;
        }
        break;
      }
      *dst++ = c;
      --n;
      if(c == '\n')
        break;
    }
    release(&uartlock1.lock);
  }


  else if (ip->minor == 2) {
    acquire(&uartlock2.lock);
    while(n > 0){
      while(uart_input2.r == uart_input2.w){
        if(myproc()->killed){
          release(&uartlock2.lock);
          ilock(ip);
          return -1;
        }
        sleep(&uart_input2.r, &uartlock2.lock);
      }
      c = uart_input2.buf[uart_input2.r++ % INPUT_BUF];
      if(c == C('D')){  // EOF
        if(n < target){
          // Save ^D for next time, to make sure
          // caller gets a 0-byte result.
          uart_input2.r--;
        }
        break;
      }
      *dst++ = c;
      --n;
      if(c == '\n')
        break;
    }
    release(&uartlock2.lock);
  }

  ilock(ip);
  return target - n;
}

int
uartwrite(struct inode *ip, char *buf, int n)
{
  int i;
  iunlock(ip);

  if (ip->minor == 1) {
    acquire(&uartlock1.lock);
    for(i = 0; i < n; i++)
      uartputc(buf[i] & 0xff);
    release(&uartlock1.lock);
  }

  else if (ip->minor == 2) {
    acquire(&uartlock2.lock);
    for(i = 0; i < n; i++)
      uartputc(buf[i] & 0xff);
    release(&uartlock2.lock);
  }

  ilock(ip);
  return n;
}


void
uartinit(void)
{
  initlock(&portlock.lock, "inbound_port");
  initlock(&uartlock1.lock, "uart (com1)");

  devsw[COM1].write = uartwrite;
  devsw[COM1].read = uartread;

  uartlock1.locking = 1;

  char *p;

  // Turn off the FIFO
  outb(COM1_PORT+2, 0);

  // 9600 baud, 8 data bits, 1 stop bit, parity off.
  outb(COM1_PORT+3, 0x80);    // Unlock divisor
  outb(COM1_PORT+0, 115200/9600);
  outb(COM1_PORT+1, 0);
  outb(COM1_PORT+3, 0x03);    // Lock divisor, 8 data bits.
  outb(COM1_PORT+4, 0);
  // outb(COM1_PORT+1, 0x01);    // Enable receive interrupts.
  outb(COM1_PORT+1, 0x03);    // Enable receive and transmit interrupts.


  // If status is 0xFF, no serial port.
  if(inb(COM1_PORT+5) == 0xFF)
    return;
  uart = 1;

  // Acknowledge pre-existing interrupt conditions;
  // enable interrupts.
  inb(COM1_PORT+2);
  inb(COM1_PORT+0);
  ioapicenable(IRQ_COM1, 0); 

  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p);

  // copy-paste for COM2
  initlock(&uartlock2.lock, "uart (com1)");

  devsw[COM2].write = uartwrite;
  devsw[COM2].read = uartread;

  uartlock2.locking = 1;

  // char *p;
  // Turn off the FIFO
  outb(COM2_PORT+2, 0);

  // 9600 baud, 8 data bits, 1 stop bit, parity off.
  outb(COM2_PORT+3, 0x80);    // Unlock divisor
  outb(COM2_PORT+0, 115200/9600);
  outb(COM2_PORT+1, 0);
  outb(COM2_PORT+3, 0x03);    // Lock divisor, 8 data bits.
  outb(COM2_PORT+4, 0);
  // outb(COM2_PORT+1, 0x01);    // Enable receive interrupts.
  outb(COM2_PORT+1, 0x03);    // Enable receive and transmit interrupts.

  // If status is 0xFF, no serial port.
  if(inb(COM2_PORT+5) == 0xFF)
    return;
  uart = 1;

  // Acknowledge pre-existing interrupt conditions;
  // enable interrupts.
  inb(COM2_PORT+2);
  inb(COM2_PORT+0);
  ioapicenable(IRQ_COM2, 0); 

  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p);
}


//uartintr reads any waiting input characters from the UART hardware
//and forwards them to console driver (by calling consoleintr)

//takes in com parameter: to determine whether COM 1 or 2 requested interrupt
void
uartintr(int com)
{
  // int c = uartgetc();
  // cprintf("unartintr called! %d \n", c);
  // uartputc(c);
  // consoleintr(uartgetc);

  int c, doprocdump = 0;

  //COM1 requested the interrupt
  if (com==1) {

    if (inb(COM1_PORT+2) & 0x2) {
      #ifdef DEBUG_PRINT
      cprintf("transmitter interrupt\n");
      #endif
      busy1=0;
    }

    if (inb(COM1_PORT+2) & 0x4) {
      #ifdef DEBUG_PRINT
      cprintf("receiver interrupt\n");
      #endif
    }

    acquire(&uartlock1.lock);
    while((c = uartgetc()) >= 0){
      switch(c){
      case C('P'):  // Process listing.
        // procdump() locks cons.lock indirectly; invoke later
        doprocdump = 1;
        break;
      case C('U'):  // Kill line.
        while(uart_input1.e != uart_input1.w &&
              uart_input1.buf[(uart_input1.e-1) % INPUT_BUF] != '\n'){
          uart_input1.e--;
          uartputc(BACKSPACE);
        }
        break;
      case C('H'): case '\x7f':  // Backspace
        if(uart_input1.e != uart_input1.w){
          uart_input1.e--;
          uartputc(BACKSPACE);
        }
        break;
      default:
        if(c != 0 && uart_input1.e-uart_input1.r < INPUT_BUF){
          c = (c == '\r') ? '\n' : c;
          uart_input1.buf[uart_input1.e++ % INPUT_BUF] = c;
          uartputc(c);
          if(c == '\n' || c == C('D') || uart_input1.e == uart_input1.r+INPUT_BUF){
            uart_input1.w = uart_input1.e;
            wakeup(&uart_input1.r);
          }
        }
        break;
      }
    }
    release(&uartlock1.lock);
    if(doprocdump) {
      procdump();  // now call procdump() wo. cons.lock held
    }
  }


  //COM2 requested an interrupt
  else if (com==2) {
    if (inb(COM2_PORT+2) & 0x2) {
      #ifdef DEBUG_PRINT
      cprintf("transmitter interrupt\n");
      #endif
      busy2=0;
    }

    if (inb(COM2_PORT+2) & 0x4) {
      #ifdef DEBUG_PRINT
      cprintf("receiver interrupt\n");
      #endif
    }

    acquire(&uartlock2.lock);
    while((c = uartgetc()) >= 0){
      switch(c){
      case C('P'):  // Process listing.
        // procdump() locks cons.lock indirectly; invoke later
        doprocdump = 1;
        break;
      case C('U'):  // Kill line.
        while(uart_input2.e != uart_input2.w &&
              uart_input2.buf[(uart_input2.e-1) % INPUT_BUF] != '\n'){
          uart_input2.e--;
          uartputc(BACKSPACE);
        }
        break;
      case C('H'): case '\x7f':  // Backspace
        if(uart_input2.e != uart_input2.w){
          uart_input2.e--;
          uartputc(BACKSPACE);
        }
        break;
      default:
        if(c != 0 && uart_input2.e-uart_input2.r < INPUT_BUF){
          c = (c == '\r') ? '\n' : c;
          uart_input2.buf[uart_input2.e++ % INPUT_BUF] = c;
          uartputc(c);
          if(c == '\n' || c == C('D') || uart_input2.e == uart_input2.r+INPUT_BUF){
            uart_input2.w = uart_input2.e;
            wakeup(&uart_input2.r);
          }
        }
        break;
      }
    }
    release(&uartlock2.lock);
    if(doprocdump) {
      procdump();  // now call procdump() wo. cons.lock held
    }
  }


}

