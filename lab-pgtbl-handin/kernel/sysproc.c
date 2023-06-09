#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
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



int
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
#ifdef LAB_PGTBL
uint64 sys_pgaccess(void){
//void *base, int len, void *mask
  uint64 va;
  int numPages;
  uint64 bitmask;
  uint64 ans = 0;

  argaddr(0, &va);
  argint(1, &numPages);
  if(numPages > 64)
     return -1;
  argaddr(2, &bitmask);
  for (int i = 0; i < numPages; i++){
    pte_t* pte = walk(myproc()->pagetable, va + i * PGSIZE, 0);
   // printf("in  syscall\n");
    if(*pte & PTE_A){
      ans = ans | (1 << i);
      *pte = *pte ^ PTE_A;
     // printf("%d in  syscall\n", ans);
    }
  }
  return copyout(myproc()->pagetable, bitmask, (char*) &ans, sizeof(ans));
}
#endif