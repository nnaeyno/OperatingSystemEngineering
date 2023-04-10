// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define numBuckets 13

struct {
  struct spinlock lock[numBuckets]; //lock each bucket individually
  struct buf buf[NBUF];
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf hash[numBuckets]; // have buckets instead of one linked list
} bcache;

void
binit(void)
{
  struct buf *b;
  int index = 0;
  //initlock(&bcache.lock, "bcache");
  //init locks and arrays
  for (int i = 0; i < numBuckets; i++){
    bcache.hash[i].prev = &bcache.hash[i];
    bcache.hash[i].next = &bcache.hash[i];
    initlock(&bcache.lock[i], "bcache");
  }
  
  // Create linked list of buffers

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.hash[index].next;
    b->prev = &bcache.hash[index];
    initsleeplock(&b->lock, "buffer"); //one buffer one lock
    bcache.hash[index].next->prev = b;
    bcache.hash[index].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int hashInd = blockno % numBuckets;
  acquire(&bcache.lock[hashInd]);


  // Is the block already cached?
  for(b = bcache.hash[hashInd].next; b != &bcache.hash[hashInd]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hashInd]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  //Not cached. 
  //in the same bucket case

  //printf("----------------------------------\n");
  //in diff bucket
  //struct buf *freehead;
  for(int i = 0; i < numBuckets; i++){
    if(i == hashInd)
      continue;
    acquire(&bcache.lock[i]);
    for(b = (&bcache.hash[(blockno + i) % numBuckets])->prev; b != &bcache.hash[(blockno + i) % numBuckets]; b = b->prev) {
      //printf("here2\n");
      if(b->refcnt == 0) {
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        // was before

        struct buf *prev = b->prev;
        b->next->prev = prev;
        prev->next = b->next;

        b->next = (&bcache.hash[hashInd])->next;
        b->prev = &bcache.hash[hashInd];
       (&bcache.hash[hashInd])->next->prev = b;
       (&bcache.hash[hashInd])->next = b;

        release(&bcache.lock[i]);

        release(&bcache.lock[hashInd]);
        acquiresleep(&b->lock);
        //was before
        return b;
      }    
    }
    release(&bcache.lock[i]);
  }
  
  /*
    for(int i = 0; i < numBuckets; i++){
      acquire(&bcache.lock[i]);
       struct buf *prev = struct buf *prev;
        prev = b->prev;
        prev->next = b->next;
        b->next = bcache.hash[i].next;
        bcache.hash[i].next = b;

      release(&bcache.lock[i]);
      }
  */
  panic("bget: no buffers");

}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  int hashInd = b->blockno % numBuckets;
  releasesleep(&b->lock);

  acquire(&bcache.lock[hashInd]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hash[hashInd].next;
    b->prev = &bcache.hash[hashInd];
    bcache.hash[hashInd].next->prev = b;
    bcache.hash[hashInd].next = b;
  }
  
  release(&bcache.lock[hashInd]);
}

void
bpin(struct buf *b) {
  int hashInd = b->blockno % numBuckets;
  acquire(&bcache.lock[hashInd]);
  b->refcnt++;
  release(&bcache.lock[hashInd]);
}

void
bunpin(struct buf *b) {
  int hashInd = b->blockno % numBuckets;
  acquire(&bcache.lock[hashInd]);
  b->refcnt--;
  release(&bcache.lock[hashInd]);
}


