// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

struct{
  int ptes[MAXNUM];
  struct spinlock lock; //the array is shared memory  
} referenceCount;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&referenceCount.lock, "refCount");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    acquire(&kmem.lock);
    referenceCount.ptes[(uint64)p/PGSIZE] = 1; //set up
    release(&kmem.lock);
    kfree(p);
  }
}

int changeReferenceCount(uint64 pa, int i){
    int res = 0;
    acquire(&kmem.lock);
    res = referenceCount.ptes[pa / PGSIZE] + i;
    referenceCount.ptes[pa / PGSIZE]+=i;
    release(&kmem.lock);
    return res;

}
// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  int num = changeReferenceCount((uint64)pa, -1);
  if(num < 0)
    panic("kfree");
  if(num > 0)
    return;
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
  
    referenceCount.ptes[(uint64)r/PGSIZE] = 1; //set up

  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
