#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"
#include "sound.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}

//PAGEBREAK: 41
int t = 0;
extern void incrementstats(void);
extern void printstats(int);
extern void calc_avg(void);
extern void calc_latency(void);
extern int is_cpu_idle(void);

void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed)
      exit();
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed)
      exit();
    return;
  }
  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:

    calc_latency();
    incrementstats();

    //produces printout on console every 1000 ticks (~10 sec)
    t++;
    //calculate sample every second
    if (t % 100 == 0){
      calc_avg();
    }
    // if (t % 1000 == 0) {
    //   //cprintf("timer interrupt!\n");
    //   printstats(t);
    // } 

    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      beep_timer();
      wakeup(&ticks);
      release(&tickslock);
    }
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE:
    // cprintf("IDE1 trap");
    ideintr(BASE_ADDR1, BASE_ADDR2);
    lapiceoi();
    break;
  /* 
    // Commented this out. Don't know if we would have to comment it back later.
    case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break; */

  //HW5
  case T_IRQ0 + IRQ_IDE2:
    // cprintf("IDE2 trap");
    ideintr(BASE_ADDR3, BASE_ADDR4);
    lapiceoi();
    break;

  case T_IRQ0 + IRQ_KBD:
    kbdintr();
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM1:
    //interrupts from serial port COM1
    uartintr(1);
    lapiceoi();
    break;
  case T_IRQ0 + IRQ_COM2:
    //interrupts from serial port COM2
    uartintr(2);
    lapiceoi();
    break;
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
  case T_IRQ0 + IRQ_SPURIOUS1:
    cprintf("cpu%d: spurious interrupt (%d) at %x:%x\n",
            cpuid(), tf->trapno, tf->cs, tf->eip);
    lapiceoi();
    break;
  case T_PGFLT:
    cprintf("overflow handler in trap.c, pgdir: %d, size: %d, kstack: %p\n",*myproc()->pgdir,myproc()->sz,myproc()->kstack);

    uint vaddr = rcr2();
    if (vaddr < 0x7FC00000) {
      //more than 4MB below KERNBASE (0x7FC00000 = KERNBASE-4MB)
      //freevm(myproc()->pgdir);
      myproc()->killed = 1;
      break;
    }

    if (vaddr >= KERNBASE) {
      //above "low part" of process address space
      //freevm(myproc()->pgdir);
      myproc()->killed = 1;
      break;
    }

    //assume myproc is not 0
    //if less than 4 megabytes
    if ((myproc()->stack_pages * PGSIZE) < 4194304) {
      uint newsz = KERNBASE - (myproc()->stack_pages * PGSIZE);
      uint oldsz = newsz - PGSIZE;

      if (allocuvm(myproc()->pgdir, oldsz, newsz) == 0) {
        //something went wrong
        // cprintf("something went wrong when incrementing user memory\n");
        //freevm(myproc()->pgdir);
        myproc()->killed = 1;
        break;
      }
      myproc()->stack_pages++;
      // cprintf("Number of pages: %d\n", myproc()->stack_pages);
    }
    else {
      //4MB exceeded, kill the process
      cprintf("4MB exceeded\n");
      //freevm(myproc()->pgdir);
      myproc()->killed = 1;
    }
    break;

  //PAGEBREAK: 13
  default:
    if(myproc() == 0 || (tf->cs&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
              tf->trapno, cpuid(), tf->eip, rcr2());
      //panic("trap");
      return;
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d err %d on cpu %d "
            "eip 0x%x addr 0x%x--kill proc\n",
            myproc()->pid, myproc()->name, tf->trapno,
            tf->err, cpuid(), tf->eip, rcr2());
    myproc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running
  // until it gets to the regular system call return.)
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();

  // Previously: Invoke the scheduler on clock tick.
  // if(tf->trapno == T_IRQ0+IRQ_TIMER) {
  //   reschedule();
  // }

  // Invoke the scheduler every QUANTUM (100) ticks,
  // if(tf->trapno == T_IRQ0+IRQ_TIMER && t % QUANTUM == 0) {
  //   reschedule();
  // }

  // Invoke the scheduler every QUANTUM (100) ticks,
  // or if there is currently an idle CPU
  if ((tf->trapno == T_IRQ0+IRQ_TIMER) && (is_cpu_idle() || t % QUANTUM == 0)) {
    reschedule();
  }


  // Check if the process has been killed since we yielded
  if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
    exit();
}
