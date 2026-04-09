#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "proc.c"

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
  int target_pid;
  int value;
  struct proc *p;
  struct proc *curr_proc = myproc();

  argint(0, &target_pid);
  argint(1, &value);

  if (target_pid < 1 || target_pid == curr_proc->pid) {
    return -1;
  }

  for (p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    // if it's the target, it's sleeping and the target called co_yield on me
    // the casting to void* is for the comparison to chan
    if (p->pid == target_pid) {
      if (p->state == SLEEPING && p->chan == (void*)(uint64)curr_proc->pid) {
        p->trapframe->a0 = value;
        acquire(&curr_proc->lock);
        curr_proc->chan = (void*)(uint64)target_pid;
        curr_proc->state = SLEEPING;
        p->state = RUNNING;

        swtch(&curr_proc->context, &p->context);
        release(&curr_proc->lock);
        release(&p->lock);
        return curr_proc->trapframe->a0;
      }
      release(&p->lock);
      break;
    }
    release(&p->lock);
  }

  acquire(&curr_proc->lock);
  curr_proc->chan = (void*)(uint64)target_pid;
  curr_proc->state = SLEEPING;
  sched();
  release(&curr_proc->lock);
  return curr_proc->trapframe->a0;
}
