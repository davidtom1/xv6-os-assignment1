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
  int found = 0;
  struct proc *p;
  struct proc *curr_proc = myproc();
  argint(0, &target_pid);
  argint(1, &value);

  if (target_pid <= 0 || target_pid == curr_proc->pid || value < 0)
    return -1;

  // Mark ourselves as sleeping BEFORE searching for the target.
  // This closes the race where the target runs between us releasing its lock
  // and us calling sched(), sees us not sleeping, and goes to sleep itself —
  // leaving both processes sleeping with nobody to wake either.
  acquire(&curr_proc->lock);
  curr_proc->chan = (void*)(uint64)target_pid;
  curr_proc->state = SLEEPING;

  for (p = proc; p < &proc[NPROC]; p++) {
    if (p == curr_proc) continue;  // already hold our own lock; can't re-acquire
    acquire(&p->lock);
    if (p->pid == target_pid) {
      found = 1;
      if (p->killed || p->state == ZOMBIE) {
        // Undo sleep setup and bail out.
        curr_proc->chan = 0;
        curr_proc->state = RUNNING;
        release(&p->lock);
        release(&curr_proc->lock);
        return -1;
      }
      if (p->state == SLEEPING && p->chan == (void*)(uint64)curr_proc->pid) {
        // Target is waiting for us: deliver our value and make it runnable.
        // The scheduler will resume it; we sleep via sched() below.
        p->trapframe->a0 = value;
        p->state = RUNNABLE;
      }
      release(&p->lock);
      break;
    }
    release(&p->lock);
  }

  if (!found) {
    curr_proc->chan = 0;
    curr_proc->state = RUNNING;
    release(&curr_proc->lock);
    return -1;
  }

  // Sleep until the target calls co_yield back to us and sets our return value.
  sched();
  curr_proc->chan = 0;
  if (curr_proc->killed) {
    release(&curr_proc->lock);
    return -1;
  }
  release(&curr_proc->lock);
  return curr_proc->trapframe->a0;
}
