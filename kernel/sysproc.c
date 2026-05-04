#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"


uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// pass the CPU for other process (for it's co_yield)
// return the value from the target process
uint64
sys_co_yield(void)
{
  extern struct proc proc[];
  int target_pid; 
  int value;
  struct proc *p;
  struct proc *target = 0;
  struct proc *curr = myproc();

  argint(0, &target_pid);
  argint(1, &value);

  if (target_pid <= 0 || target_pid == curr->pid || value < 0)
    return -1;

  // Locate target and hld its lock.
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p == curr) 
      continue;
    acquire(&p->lock);
    if (p->pid == target_pid) {
      target = p;
      break;
    }
    release(&p->lock);
  }
  
  if (target == 0)
    return -1;

  if (target->killed || target->state == ZOMBIE) {
    release(&target->lock);
    return -1;
  }

  // Target is already sleeping & waiting for us - direct context switch
  if (target->state == SLEEPING && target->chan == (void*)(uint64)curr->pid) {
    acquire(&curr->lock);

    // Hand value to target to its trapframe.
    target->trapframe->a0 = value;
    target->state = RUNNING;        
    target->chan  = 0;

    // Curr sleeps on target_pid; whoever yields back to curr will find curr here.
    curr->state = SLEEPING;
    curr->chan  = (void*)(uint64)target_pid;

    // Tell the CPU it's now running target.
    mycpu()->proc = target;

    release(&curr->lock);

    int intena = mycpu()->intena;
    swtch(&curr->context, &target->context);
    mycpu()->intena = intena;

    // Resumed here when someone yields back to us via the same direct path.
    // They held curr->lock when swtching in, so we hold it now.
    int killed = curr->killed;
    int ret    = curr->trapframe->a0;
    release(&curr->lock);
    return killed ? -1 : ret;
  }

  // ---------------------------------------------------------------
  // EDGE PATH: target isn't ready (RUNNABLE, or sleeping on something
  // else). Fall back to standard scheduler-based sleep. sched() ONLY here.
  // When target eventually reaches its co_yield, IT will hit the happy
  // path and direct-swtch into us.
  // ---------------------------------------------------------------
  release(&target->lock);

  acquire(&curr->lock);
  if (curr->killed) { 
    release(&curr->lock); 
    return -1; 
  }

  curr->chan  = (void*)(uint64)target_pid;
  curr->state = SLEEPING;

  sched();   // <-- only call site

  curr->chan = 0;
  if (curr->killed) { 
    release(&curr->lock); 
    return -1; 
  }
  uint64 ret = curr->trapframe->a0;
  release(&curr->lock);
  return ret;
}
