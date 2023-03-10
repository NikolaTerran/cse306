// Intel 8250 serial port (UART).

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"


#define COM1_PORT    0x3f8
#define COM2_PORT    0x2f8

static int uart;    // is there a uart?

static struct {
  struct spinlock lock;
  int locking;
} uartlock;

#define BACKSPACE 0x100
#define INPUT_BUF 128

//buffer for accumulating input chars arriving on COM1/2
struct {
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} uart_input;

#define C(x)  ((x)-'@')  // Control-x


void
uartputc(int c)
{
  // int i;

  // if(!uart)
  //   return;
  // // On my test system, Bochs (running at 1M instr/sec)
  // // requires between 500 and 1000 iterations to avoid
  // // output overrun, as indicated by log messages in the
  // // Bochs log file.  This is with microdelay() having an
  // // empty body and ignoring its argument.
  // for(i = 0; i < 1000 && !(inb(COM1_PORT+5) & 0x20); i++)
  //   microdelay(10);

  // outb(COM1_PORT+0, c); 
  
  //copy-paste for com2
  int i;

  if(!uart)
    return;
  for(i = 0; i < 1000 && !(inb(COM2_PORT+5) & 0x20); i++)
    microdelay(10);

  outb(COM2_PORT+0, c); 
}

static int
uartgetc(void)
{
  // if(!uart)
  //   return -1;
  // if(!(inb(COM1_PORT+5) & 0x01))
  //   return -1;
  // return inb(COM1_PORT+0);
  //copy-paste for com2

  if(!uart)
    return -1;
  if(!(inb(COM2_PORT+5) & 0x01))
    return -1;
  return inb(COM2_PORT+0);
}

int
uartread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&uartlock.lock);
  while(n > 0){
    while(uart_input.r == uart_input.w){
      if(myproc()->killed){
        release(&uartlock.lock);
        ilock(ip);
        return -1;
      }
      sleep(&uart_input.r, &uartlock.lock);
    }
    c = uart_input.buf[uart_input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        uart_input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&uartlock.lock);
  ilock(ip);

  return target - n;
}

int
uartwrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&uartlock.lock);
  for(i = 0; i < n; i++)
    uartputc(buf[i] & 0xff);
  release(&uartlock.lock);
  ilock(ip);

  return n;
}


void
uartinit(void)
{

  // initlock(&uartlock.lock, "uart (com1)");

  // devsw[COM1].write = uartwrite;
  // devsw[COM1].read = uartread;

  // uartlock.locking = 1;

  // char *p;

  // // Turn off the FIFO
  // outb(COM1_PORT+2, 0);

  // // 9600 baud, 8 data bits, 1 stop bit, parity off.
  // outb(COM1_PORT+3, 0x80);    // Unlock divisor
  // outb(COM1_PORT+0, 115200/9600);
  // outb(COM1_PORT+1, 0);
  // outb(COM1_PORT+3, 0x03);    // Lock divisor, 8 data bits.
  // outb(COM1_PORT+4, 0);
  // outb(COM1_PORT+1, 0x01);    // Enable receive interrupts.

  // // If status is 0xFF, no serial port.
  // if(inb(COM1_PORT+5) == 0xFF)
  //   return;
  // uart = 1;

  // // Acknowledge pre-existing interrupt conditions;
  // // enable interrupts.
  // inb(COM1_PORT+2);
  // inb(COM1_PORT+0);
  // ioapicenable(IRQ_COM1, 0); 

  // // Announce that we're here.
  // for(p="xv6...\n"; *p; p++)
  //   uartputc(*p);

  // copy-paste for COM2
  initlock(&uartlock.lock, "uart (com1)");

  devsw[COM2].write = uartwrite;
  devsw[COM2].read = uartread;

  uartlock.locking = 1;

  char *p;
  // Turn off the FIFO
  outb(COM2_PORT+2, 0);

  // 9600 baud, 8 data bits, 1 stop bit, parity off.
  outb(COM2_PORT+3, 0x80);    // Unlock divisor
  outb(COM2_PORT+0, 115200/9600);
  outb(COM2_PORT+1, 0);
  outb(COM2_PORT+3, 0x03);    // Lock divisor, 8 data bits.
  outb(COM2_PORT+4, 0);
  outb(COM2_PORT+1, 0x01);    // Enable receive interrupts.

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
void
uartintr(void)
{
  // int c = uartgetc();
  // cprintf("unartintr called! %d \n", c);
  // uartputc(c);
  // consoleintr(uartgetc);


  int c, doprocdump = 0;

  // cprintf("unartintr called! %d \n", doprocdump);

  acquire(&uartlock.lock);
  while((c = uartgetc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'):  // Kill line.
      while(uart_input.e != uart_input.w &&
            uart_input.buf[(uart_input.e-1) % INPUT_BUF] != '\n'){
        uart_input.e--;
        uartputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if(uart_input.e != uart_input.w){
        uart_input.e--;
        uartputc(BACKSPACE);
      }
      break;
    default:
      if(c != 0 && uart_input.e-uart_input.r < INPUT_BUF){
        c = (c == '\r') ? '\n' : c;
        uart_input.buf[uart_input.e++ % INPUT_BUF] = c;
        uartputc(c);
        if(c == '\n' || c == C('D') || uart_input.e == uart_input.r+INPUT_BUF){
          uart_input.w = uart_input.e;
          wakeup(&uart_input.r);
        }
      }
      break;
    }
  }
  release(&uartlock.lock);
  if(doprocdump) {
    procdump();  // now call procdump() wo. cons.lock held
  }


}

