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



#define BUCKETNUM 13

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf head[BUCKETNUM];//桶
  struct spinlock bucket_lock[BUCKETNUM];
} bcache; 

char lockname[BUCKETNUM][10];

int hash(int blockno){
	return blockno % BUCKETNUM;
}

void
binit(void)
{
  struct buf *b;
  
  initlock(&bcache.lock, "bcache_big");
  for(int i = 0; i < BUCKETNUM; i++){
        snprintf(lockname[i],10,"bcache%d",i);
  	initlock(&bcache.bucket_lock[i], lockname[i]);
        bcache.head[i].next = 0;
  }

  // Create linked list of buffers
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head[0].next;
    bcache.head[0].next = b;
    initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int h = hash(blockno);

  //1. find
  acquire(&bcache.bucket_lock[h]);
  for(b = bcache.head[h].next; b != 0; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.bucket_lock[h]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  
  //2. not catch
  acquire(&bcache.lock);
refind:  
  uint64 min_time = ~0;
  struct buf *replace_buf = 0;

  for(b = bcache.buf; b < bcache.buf + NBUF; b++){
  	if(b->refcnt == 0 && b->timestamp < min_time){
  		replace_buf = b;
  		min_time = b->timestamp;
  	}
  }
  
  if(replace_buf){
  	int oldh = hash(replace_buf->blockno);
  	if(oldh != h){
	  	acquire(&bcache.bucket_lock[oldh]);
	  	/*	
	  	if(replace_buf->refcnt != 0){
	  	
	  		release(&bcache.bucket_lock[oldh]);
	  		
	  		goto refind;
	  	}
	  	*/
	  	
	  	struct buf* pre = &bcache.head[oldh];
	  	struct buf* p = bcache.head[oldh].next;
	  	
	  	while(p != replace_buf){
	  		pre = pre->next;
	  		p = p->next;
	  	}
	  	
	  	pre->next = p->next;
	  	release(&bcache.bucket_lock[oldh]);
	  	
	  	replace_buf->next = bcache.head[h].next;
	  	bcache.head[h].next = replace_buf;
	  	
	  	release(&bcache.lock);
	  	
	  	replace_buf->dev = dev;
	  	replace_buf->blockno = blockno;
	  	replace_buf->valid = 0;
	  	replace_buf->refcnt = 1;
	  	release(&bcache.bucket_lock[h]);
	  	acquiresleep(&replace_buf->lock);
	  	
	  	return replace_buf;
	  }else{
	  	if(replace_buf->refcnt != 0){
	  		goto refind;
	  	}
	  	
	  	replace_buf->dev = dev;
	  	replace_buf->blockno = blockno;
	  	replace_buf->valid = 0;
	  	replace_buf->refcnt = 1;
	  	release(&bcache.bucket_lock[h]);
	  	release(&bcache.lock);
	  	acquiresleep(&replace_buf->lock);
	  	return replace_buf;  
	  }
  }
  release(&bcache.lock);
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
  
  releasesleep(&b->lock);

  
  int h = hash(b->blockno);

  acquire(&bcache.bucket_lock[h]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    //b->next->prev = b->prev;
    //b->prev->next = b->next;
    //b->next = bcache.head[hashcode].next;
   //b->prev = &bcache.head[hashcode];
    //bcache.head[hashcode].next->prev = b;
    //bcache.head[hashcode].next = b;
    
    b->timestamp = ticks;
  }
  release(&bcache.bucket_lock[h]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.bucket_lock[hash(b->blockno)]);
  b->refcnt++;
  release(&bcache.bucket_lock[hash(b->blockno)]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.bucket_lock[hash(b->blockno)]);
  b->refcnt--;
  release(&bcache.bucket_lock[hash(b->blockno)]);
}




