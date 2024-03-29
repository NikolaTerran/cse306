#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define CPU_SCHEDULER 0

static struct proc *roundrobin();
static struct proc *lowestcpu();
static struct proc *highestwait();

static struct proc *(*scheduler[])() = {
  [0]    roundrobin,
  [1]    lowestcpu,
  [2]    highestwait
};

static char *scheduler_str[] = {
  [0]    "roundrobin",
  [1]    "lowest CPU\% first",
  [2]    "highest wait\% first"
};

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);
static void sched(void);
static struct proc *roundrobin(void);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  np->stack_pages = curproc->stack_pages;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU idle loop.
// Each CPU calls idle() after setting itself up.
// Idle never returns.  It loops, executing a HLT instruction in each
// iteration.  The HLT instruction waits for an interrupt (such as a
// timer interrupt) to occur.  Actual work gets done by the CPU when
// the scheduler is invoked to switch the CPU from the idle loop to
// a process context.
void
idle(void)
{
  sti(); // Enable interrupts on this processor
  for(;;) {
    if(!(readeflags()&FL_IF))
      panic("idle non-interruptible");
    hlt(); // Wait for an interrupt
  }
}

// The process scheduler.
//
// Assumes ptable.lock is held, and no other locks.
// Assumes interrupts are disabled on this CPU.
// Assumes proc->state != RUNNING (a process must have changed its
// state before calling the scheduler).
// Saves and restores intena because the original xv6 code did.
// (Original comment:  Saves and restores intena because intena is
// a property of this kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would break in the few
// places where a lock is held but there's no process.)
//
// When invoked, does the following:
//  - choose a process to run
//  - swtch to start running that process (or idle, if none)
//  - eventually that process transfers control
//      via swtch back to the scheduler.

static void
sched(void)
{
  int intena;
  struct proc *p;
  struct context **oldcontext;
  struct cpu *c = mycpu();
  
  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(c->ncli != 1)
    panic("sched locks");
  if(readeflags()&FL_IF)
    panic("sched interruptible");

  // Determine the current context, which is what we are switching from.
  if(c->proc) {
    if(c->proc->state == RUNNING)
      panic("sched running");
    oldcontext = &c->proc->context;
  } else {
    oldcontext = &(c->scheduler);
  }

  // Choose next process to run.
  if((p = scheduler[CPU_SCHEDULER]()) != 0) {
    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    p->state = RUNNING;
    switchuvm(p);
    if(c->proc != p) {
      c->proc = p;
      intena = c->intena;
      swtch(oldcontext, p->context);
      mycpu()->intena = intena;  // We might return on a different CPU.
    }
  } else {
    // No process to run -- switch to the idle loop.
    switchkvm();
    if(oldcontext != &(c->scheduler)) {
      c->proc = 0;
      intena = c->intena;
      swtch(oldcontext, c->scheduler);
      mycpu()->intena = intena;
    }
  }
}

// Round-robin scheduler.
// The same variable is used by all CPUs to determine the starting index.
// It is protected by the process table lock, so no additional lock is
// required.
static int rrindex;

static struct proc *
roundrobin()
{
  // Loop over process table looking for process to run.
  for(int i = 0; i < NPROC; i++) {
    struct proc *p = &ptable.proc[(i + rrindex + 1) % NPROC];
    if(p->state != RUNNABLE)
      continue;
    rrindex = p - ptable.proc;
    return p;
  }
  return 0;
}

// Lowest CPU-utilization scheduler

static struct proc *
lowestcpu()
{
  // for loop to choose the process with lowest util_avg value
  double low_util = 100;
  struct proc *p = 0;
  for(int i = 0; i < NPROC; i++) {
    if(ptable.proc[i].state != RUNNABLE)
      continue; 
    if(ptable.proc[i].util_avg < low_util) {
      p = &ptable.proc[i];
      low_util = p->util_avg;
    }
  }
  return p;
}

// Highest wait percentage first scheduler
static struct proc *
highestwait()
{
  double high_wait = 0; //current max
  struct proc *p = 0;

  // for loop to choose the process with highest wait percentage
  for (int i = 0; i < NPROC; i++) {
    if(ptable.proc[i].state != RUNNABLE)
      continue; 
    if(ptable.proc[i].wait_avg > high_wait) {
      p = &ptable.proc[i];
      high_wait = p->wait_avg;
    }
  }
  return p;

}

// Called from timer interrupt to reschedule the CPU.
void
reschedule(void)
{
  struct cpu *c = mycpu();

  acquire(&ptable.lock);
  if(c->proc) {
    if(c->proc->state != RUNNING)
      panic("current process not in running state");
    c->proc->state = RUNNABLE;
  }
  sched();
  // NOTE: there is a race here.  We need to release the process
  // table lock before idling the CPU, but as soon as we do, it
  // is possible that an an event on another CPU could cause a process
  // to become ready to run.  The undesirable (but non-catastrophic)
  // consequence of such an occurrence is that this CPU will idle until
  // the next timer interrupt, when in fact it could have been doing
  // useful work.  To do better than this, we would need to arrange
  // for a CPU releasing the process table lock to interrupt all other
  // CPUs if there could be any runnable processes.
  release(&ptable.lock);
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
    uiinit(XV5DEV);
    //unix v5 does not have a log section
    // initlog(XV5DEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


// Increments the appropriate field on each timer interrupt (running, runnable, sleeping), 
// according to the state the process is in at the time of that interrupt
// Called in trap.c, for each tick
void 
incrementstats(void) {
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state == UNUSED) {
      p->prev = 1;
      continue;
    }
    if (p->state == EMBRYO) 
      p->prev = 1;

    if (p->state == RUNNING) {
      p->running++;
      p->prev = 0;
    }

    if (p->state == RUNNABLE) {
      p->runnable++;
      p->prev = 0;
    }

    if (p->state == SLEEPING) {
      p->sleeping++;
      p->prev = 1;
    }
  }
  return;
}


// Function that prints out stats for each process (how long process
// is running, waiting for CPU, or sleeping)
// Called in trap.c, for every 1000 ticks

// Some parts copied directly from procdump()
static double avg = 0;
#define CONSTANT 0.96
#define CPU_CONS 0.93
#define WAIT_CONS 0.93
#define WAKEUP_CONS 0.93

void calc_latency() {
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->prev && p->state == RUNNABLE) {
      p->in=1;
    }

    else if (p->in && p->state == RUNNABLE) {
      p->curr_latency++;
    }

    else if (p->in && p->state == RUNNING) {
      p->in=0;
      if (p->curr_latency > p->max_latency)
        p->max_latency = p->curr_latency;

      p->curr_latency = 0;
    }

  }
}

void calc_avg(){
  struct proc *p;
  double sample = 0.0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    //running or runnable
    if(p->state == 3 || p->state == 4){
      sample += 1.0;
    }
    double diff1 = (p->running - p->last_run)/100.0;
    double diff2 = (p->runnable - p->last_wait)/100.0;
    p->util_avg = CPU_CONS * p->util_avg + (1-CPU_CONS) * diff1;
    p->wait_avg = WAIT_CONS * p->wait_avg + (1-WAIT_CONS) * diff2;

    p->latency = WAKEUP_CONS * p->latency + (1-WAKEUP_CONS) * p->max_latency;
    p->max_latency = 0; //reset max for next 100 ticks

    p->last_run = p->running;
    p->last_wait = p->runnable;
  }
  avg = CONSTANT * avg + (1-CONSTANT) * sample;
}

// Determines if there is an idle CPU that could run a process
// An idle CPU means none of the processes in ptable are in the RUNNING state
int is_cpu_idle() {
  struct cpu *c = mycpu();
  if (c->proc)
    return 0;
  else
    return 1;
}

void 
printstats(int uptime) {
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  //recalculate avg
  cprintf("cpus: %d, uptime: %d, load(x100): %d, scheduler: %s\n", ncpu, uptime, (int)(avg * 100.0),scheduler_str[CPU_SCHEDULER]);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state]){
      state = states[p->state];
    }
    else
      state = "???";
    
    cprintf("%d %s %s run: %d wait: %d sleep: %d cpu%: %d wait%: %d latency: %d\n", 
      p->pid, state, p->name, p->running, p->runnable, p->sleeping, (int)(p->util_avg * 100.0), 
      (int)(p->wait_avg * 100.0), (int) p->latency);
  }
  cprintf("\n");
  return;
}