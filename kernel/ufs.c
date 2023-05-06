// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, ureading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "ufs.h"
#include "buf.h"
#include "file.h"

#define min(a, b) ((a) < (b) ? (a) : (b))
static void itrunc(struct inode*);
// // there should be one superblock per disk device, but we run with
// // only one device
// struct usuperblock usb; 

// this holds the actual superblock info
struct filsys fs;

// // Read the super block.
// // not used, see readfilsys
// void
// ureadsb(int dev, struct usuperblock *usb)
// {
//   struct buf *bp;
//   bp = bread(dev, 1);
//   memmove(usb, bp->data, sizeof(*usb));
//   brelse(bp);
// }

void
readfilsys(int dev, struct filsys *fs){
  struct buf *bp;
  bp = bread(dev, 1);
  memmove(fs, bp->data, sizeof(*fs));
  brelse(bp);
}

// Zero a block.
static void
bzero(int dev, int bno)
{
  struct buf *bp;

  bp = bread(dev, bno);
  memset(bp->data, 0, UBSIZE);
  // log_write(bp);
  bwrite(bp);
  brelse(bp);
}

// Blocks.

// // Obtain the next avaliable free disk block from the free list of the specified device
// static ushort
// balloc(uint dev)
// {
//   short b, bi, m;
//   struct buf *bp;

//   bp = 0;
//   //100 is the size of fs.s_free
//   for(b = 0; b < 100; b += 1){
//     bp = bread(dev, fs.s_free[b]);
//     for(bi = 0; bi < UBPB && b + bi < fs.s_fsize * 512; bi++){
//       m = 1 << (bi % 8);
//       if((bp->data[bi/8] & m) == 0){  // Is block free?
//         cprintf("block is free\n");
//         bp->data[bi/8] |= m;  // Mark block in use.
//         // log_write(bp);
//         bwrite(bp);
//         brelse(bp);
//         bzero(dev, b + bi);
//         return b + bi;
//       }
//     }
//     brelse(bp);
//   }

//   panic("balloc: out of blocks");
// }

// Free a disk block.
// static void
// bfree(int dev, uint b)
// {
//   struct buf *bp;
//   int bi, m;

//   //ureadsb(dev, &usb);
//   readfilsys(dev, &fs);
//   bp = bread(dev, fs.s_free[0] + b);
//   bi = b % UBPB;
//   m = 1 << (bi % 8);
//   if((bp->data[bi/8] & m) == 0)
//     panic("freeing free block");
//   bp->data[bi/8] &= ~m;
//   // log_write(bp);
//   bwrite(bp);
//   brelse(bp);
// }

static ushort ubadblock(uint abn, ushort dev)
{
	struct filsys *fp;
	char * bn;
	fp = &fs;
	bn = (char *)abn;
	if (bn < (char *)(fp->s_isize+2) || bn >= (char *)(uint)(fp->s_fsize)) {
		panic("bad block");
	}
	return(0);
}


static void bfree(ushort dev, uint bno)
{
	ushort *bp, *ip;
	fs.s_fmod = 1;
  // don't sleep
	// while(fp->s_flock)
	// 	sleep(&fp->s_flock, PINOD);
	if (ubadblock(bno, dev))
		return;
	if(fs.s_nfree >= 100) {
		fs.s_flock++;
		bp = (ushort *)bread(dev, bno);
		ip = (ushort *)(((struct v5buf *)bp)->b_addr);
		*ip++ = fs.s_nfree;
		
    //bcopy(fs.s_free, ip, 100);
    memmove(fs.s_free, ip, 50);

		fs.s_nfree = 0;
		bwrite((struct buf*)bp);
		fs.s_flock = 0;
		wakeup(&fs.s_flock);
	}
	fs.s_free[fs.s_nfree++] = bno;
	fs.s_fmod = 1;
}

// Inodes.
//
// An inode describes a single unnamed file.
// The inode disk structure holds metadata: the file's type,
// its size, the number of links referring to it, and the
// list of blocks holding the file's content.
//
// The inodes are laid out sequentially on disk at
// usb.startinode. Each inode has a number, indicating its
// position on the disk.
//
// The kernel keeps a cache of in-use inodes in memory
// to provide a place for synchronizing access
// to inodes used by multiple processes. The cached
// inodes include book-keeping information that is
// not stored on disk: ip->ref and ip->valid.
//
// An inode and its in-memory representation go through a
// sequence of states before they can be used by the
// rest of the file system code.
//
// * Allocation: an inode is allocated if its type (on disk)
//   is non-zero. ialloc() allocates, and iput() frees if
//   the reference and link counts have fallen to zero.
//
// * Referencing in cache: an entry in the inode cache
//   is free if ip->ref is zero. Otherwise ip->ref tracks
//   the number of in-memory pointers to the entry (open
//   files and current directories). iget() finds or
//   creates a cache entry and increments its ref; iput()
//   decrements ref.
//
// * Valid: the information (type, size, &c) in an inode
//   cache entry is only correct when ip->valid is 1.
//   ilock() reads the inode from
//   the disk and sets ip->valid, while iput() clears
//   ip->valid if ip->ref has fallen to zero.
//
// * Locked: file system code may only examine and modify
//   the information in an inode and its content if it
//   has first locked the inode.
//
// Thus a typical sequence is:
//   ip = iget(dev, inum)
//   ilock(ip)
//   ... examine and modify ip->xxx ...
//   uiunlock(ip)
//   iput(ip)
//
// ilock() is separate from iget() so that system calls can
// get a long-term reference to an inode (as for an open file)
// and only lock it for short periods (e.g., in read()).
// The separation also helps avoid deadlock and races during
// pathname lookup. iget() increments ip->ref so that the inode
// stays cached and pointers to it remain valid.
//
// Many internal file system functions expect the caller to
// have locked the inodes involved; this lets callers create
// multi-step atomic operations.
//
// The uicache.lock spin-lock protects the allocation of uicache
// entries. Since ip->ref indicates whether an entry is free,
// and ip->dev and ip->inum indicate which i-node an entry
// holds, one must hold uicache.lock while using any of those fields.
//
// An ip->lock sleep-lock protects all ip-> fields other than ref,
// dev, and inum.  One must hold ip->lock in order to
// read or write that inode's ip->valid, ip->size, ip->type, &c.

struct {
  struct spinlock lock;
  struct inode inode[NINODE];
} uicache;


// //https://www.geeksforgeeks.org/bit-manipulation-swap-endianness-of-a-number/
// // not tested yet
// ushort endian_swap(ushort value){
//     ushort leftmost_byte = (value & 0x00FF) >> 0;
//     ushort left_middle_byle = (value & 0xFF00) >> 8;

//     leftmost_byte <<= 8;
//     left_middle_byle >>= 8;
 
//     return (leftmost_byte | left_middle_byle);
// }

void
uiinit(int dev)
{
  int i = 0;
  
  initlock(&uicache.lock, "uicache");
  for(i = 0; i < NINODE; i++) {
    initsleeplock(&uicache.inode[i].lock, "inode");
  }

  readfilsys(dev, &fs);
  cprintf("filsys: isize %d fsizee %d nfree %d ninode %d ronly %d\n", fs.s_isize, fs.s_fsize,
          fs.s_nfree, fs.s_ninode, fs.s_ronly);
}

static struct inode* iget(uint dev, uint inum);

static ushort balloc(ushort dev)
{
	int bno;
	ushort *bp, *ip;
	// while(fs.s_flock)
	// 	sleep(fp.s_flock, PINOD);
	do {
		bno = fs.s_free[--fs.s_nfree];
		if(bno == 0) {
			fs.s_nfree++;
			panic("no space");
		}
	} while (ubadblock(bno, dev));
	if(fs.s_nfree <= 0) {
		fs.s_flock++;
		bp = (ushort *)bread(dev, bno);
		ip = (ushort *)(((struct v5buf*)bp)->b_addr);
		fs.s_nfree = *ip++;
		
    // bcopy(ip, fs.s_free, 100);
    memmove(ip, fs.s_free, 50);

		brelse((struct buf *)bp);
		fs.s_flock = 0;
		wakeup(&fs.s_flock);
	}
	bp = (ushort * )bread(dev, bno);
	// memset(((struct buf*)bp)->data, 0, UBSIZE);
  brelse((struct buf *)bp);
  bzero(dev, bno);
	fs.s_fmod = 1;
	return bno;
}

//completely unuseable
// // ufree(dev, bno)
// // {
// // 	register *fp, *bp, *ip;
// // 	fp = getfs(dev);
// // 	fp->s_fmod = 1;
// // 	while(fp->s_flock)
// // 		sleep(&fp->s_flock, PINOD);
// // 	if (badblock(fp, bno, dev))
// // 		return;
// // 	if(fp->s_nfree >= 100) {
// // 		fp->s_flock++;
// // 		bp = getblk(dev, bno);
// // 		ip = bp->b_addr;
// // 		*ip++ = fp->s_nfree;
// // 		bcopy(fp->s_free, ip, 100);
// // 		fp->s_nfree = 0;
// // 		bwrite(bp);
// // 		fp->s_flock = 0;
// // 		wakeup(&fp->s_flock);
// // 	}
// // 	fp->s_free[fp->s_nfree++] = bno;
// // 	fp->s_fmod = 1;
// // }
// ubadblock(afp, abn, dev)
// {
// 	register struct filsys *fp;
// 	register char *bn;
// 	fp = afp;
// 	bn = abn;
// 	if (bn < fp->s_isize+2 || bn >= fp->s_fsize) {
// 		prdev("bad block", dev);
// 		return(1);
// 	}
// 	return(0);
// }
// struct inode * uialloc(dev)
// {
// 	int i, j, k, ino;
//   ushort *ip;
//   ushort *bp;
//   // no lock
// 	// while(fs->s_ilock)
// 	// 	sleep(&fp->s_ilock, PINOD);
// loop:
// 	if(fs.s_ninode > 0) {
// 		ino = fs.s_inode[--fs.s_ninode];
// 		ip = iget(dev, ino);
// 		if(((struct inode *)ip)->type == T_FILE) {
// 			for(bp = &((struct inode *)ip)->type; bp < &((struct inode *)ip)->addrs[4];){ //pointer arithmatics
// 				*bp++ = 0;
//       }
// 			fs.s_fmod = 1;
// 			return(ip);
// 		}
// 		printf("busy i\n");
// 		iput(ip);
// 		goto loop;
// 	}
// 	fs.s_ilock++;
// 	ino = 0;
// 	for(i=0; i<fs.s_isize; i++) {
// 		bp = bread(dev, i+2);
//     //this is v5dip;
// 		ip = ((struct v5buf *)bp)->b_addr;
// 		for(j=0; j<256; j=+16) {
// 			ino++;
// 			if(ip[j] != 0)
// 				continue;
// 			for(k=0; k<NINODE; k++)
// 			if(dev==inode[k].i_dev && ino==inode[k].i_number)
// 				goto cont;
// 			fs.s_inode[fs.s_ninode++] = ino;
// 			if(fs.s_ninode >= 100)
// 				break;
// 		cont:;
// 		}
// 		brelse(bp);
// 		if(fs.s_ninode >= 100)
// 			break;
// 	}
// 	if(fs.s_ninode <= 0)
// 		panic("out of inodes");
// 	fs.s_ilock = 0;
// 	wakeup(&fs.s_ilock);
// 	goto loop;
// }
// // uifree(dev, ino)
// // {
// // 	register *fp;
// // 	fp = getfs(dev);
// // 	if(fp->s_ilock)
// // 		return;
// // 	if(fp->s_ninode >= 100)
// // 		return;
// // 	fp->s_inode[fp->s_ninode++] = ino;
// // 	fp->s_fmod = 1;
// // }
// void uiupdate(struct inode* in)
// {
// 	ushort *ip;
// 	struct mount *mp;
// 	ushort *bp;
// 	if(updlock)
// 		return;
// 	updlock++;
// 	for(mp = &mount[0]; mp < &mount[NMOUNT]; mp++)
// 		if(mp->m_bufp != NULL) {
//       struct v5buf * m1 = (struct v5buf *)(mp->m_bufp);
// 			ip = (ushort *)m1->b_addr;
// 			if(((struct filsys *)ip)->s_fmod ==0 || ((struct filsys *)ip)->s_ilock!=0 ||
// 			   ((struct filsys *)ip)->s_flock!=0 || ((struct filsys *)ip)->s_ronly!=0)
// 				continue;
// 			bp = (ushort *)getblk(mp->m_dev, 1);
// 			((struct filsys *)ip)->s_fmod = 0;
// 			((struct filsys *)ip)->s_time[0] = time[0];
// 			((struct filsys *)ip)->s_time[1] = time[1];
// 			bcopy(ip, ((struct v5buf *)bp)->b_addr, 256);
// 			bwrite((struct buf *)bp);
// 		}
// 	for(ip = (ushort *)&inode[0]; ip < (ushort *)&inode[NINODE]; ip++)
// 		if((((struct v5inode *)ip)->i_flag&ILOCK) == 0) {
// 			((struct v5inode *)ip)->i_flag |= ILOCK;
// 			// iupdat(ip, time);
// 			// prele(ip);
// 		}
// 	updlock = 0;
// 	// bflush(NODEV);
// }




//PAGEBREAK!
// Allocate an inode on device dev.
// Mark it as allocated by  giving it type type.
// Returns an unlocked but allocated and referenced inode.
struct inode*
uialloc(uint dev, short type)
{
  int inum;
  struct buf *bp;
  struct v5dinode *dip;

  for (inum=1; inum < 1280; inum++) {
    // uint addr = fs.s_inode[inum];
    bp = bread(dev, UIBLOCK(inum));
    dip = (struct v5dinode*)bp->data + (inum-1)%UIPB;
    // cprintf("i_mode %d i_nlink %d i_uid %d i_gid %d i_size0 %d i_size1 %d addr %d\n",dip->i_mode,dip->i_nlink,dip->i_uid, dip->i_gid, dip->i_size0, dip->i_size1, *(dip->i_addr));

    if((dip->i_mode & IALLOC) == 0){  // a free inode
      memset(dip, 0, sizeof(*dip));

      // cprintf("inum? : %d\n",inum);
      //if dir
      if(type == T_DIR){
        dip->i_mode = IFDIR | IALLOC;
      }else{
        //if file
        // probably have to check if it is large or small
        dip->i_mode = IALLOC + 0;
      }
      
      // cprintf("dip imode : %d\n",dip->i_mode);
      // cprintf("bp imode : %d\n",((struct v5dinode*)bp->data + (inum-1)%UIPB)->i_mode);

      bwrite(bp);
      brelse(bp);
      return iget(dev, inum);
    }
    brelse(bp);

  }
  /* for(inum = 1; inum < 4000; inum++){
    bp = bread(dev, UIBLOCK(inum));
    dip = (struct v5dinode*)bp->data + (inum-1)%UIPB;
    // cprintf("i_mode %d i_nlink %d i_uid %d i_gid %d i_size0 %d i_size1 %d addr %d\n",dip->i_mode,dip->i_nlink,dip->i_uid, dip->i_gid, dip->i_size0, dip->i_size1, *(dip->i_addr));
    // cprintf("inum: %d\n",inum);
    if((dip->i_mode & IALLOC) == 0){  // a free inode
      memset(dip, 0, sizeof(*dip));
      //if dir
      if(type == 1){
        dip->i_mode = IFDIR | IALLOC;
      }else{
        //if file
        // probably have to check if it is large or small
        dip->i_mode = 0;
      }
      cprintf("start alloc bp: %d\n",bp->dev);
      bwrite(bp);
      cprintf("end alloc\n");
      brelse(bp);
      return iget(dev, inum);
    }
    brelse(bp);
  } */
  panic("uialloc: no inodes");
}


// // Copy a modified in-memory inode to disk.
// // Must be called after every change to an ip->xxx field
// // that lives on disk, since i-node cache is write-through.
// // Caller must hold ip->lock.
void
uiupdate(struct inode *ip)
{
  // panic("uiupdate not implemented");
  struct buf *bp;
  struct v5dinode *dip;
  bp = bread(ip->dev, UIBLOCK(ip->inum));
  dip = (struct v5dinode*)bp->data + (ip->inum-1)%UIPB;

  if(ip->type == T_DIR){
    dip->i_mode = IALLOC | IFDIR;
  }else if(ip->type == 0){
    dip->i_mode &= 0x7fff;
  }else{
    dip->i_mode = IALLOC;
  }

  dip->i_nlink = ip->nlink;
  dip->i_gid = 3;
  dip->i_uid = 1;
  dip->i_size0 = ip->size >> 16;
  dip->i_size1 = ip->size;
  // don't use memmove for inodes!!!
  // memmove(dip->i_addr, ip->addrs, sizeof(ip->addrs));
  for(int i = 0; i < sizeof(dip->i_addr)/sizeof(dip->i_addr[0]); i++){
      dip->i_addr[i] = (ushort)ip->addrs[i];
  }
  bwrite(bp);
  brelse(bp);
}

// Find the inode with number inum on device dev
// and return the in-memory copy. Does not lock
// the inode and does not read it from disk.
static struct inode*
iget(uint dev, uint inum)
{
  struct inode *ip, *empty;

  acquire(&uicache.lock);

  // Is the inode already cached?
  empty = 0;
  for(ip = &uicache.inode[0]; ip < &uicache.inode[NINODE]; ip++){
    if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
      ip->ref++;
      release(&uicache.lock);
      return ip;
    }
    if(empty == 0 && ip->ref == 0)    // Remember empty slot.
      empty = ip;
  }

  // Recycle an inode cache entry.
  if(empty == 0)
    panic("iget: no inodes");

  ip = empty;
  ip->dev = dev;
  ip->inum = inum;
  ip->ref = 1;
  ip->valid = 0;
  release(&uicache.lock);

  return ip;
}

// Increment reference count for ip.
// Returns ip to enable ip = idup(ip1) idiom.
struct inode*
uidup(struct inode *ip)
{
  acquire(&uicache.lock);
  ip->ref++;
  release(&uicache.lock);
  return ip;
}

// Lock the given inode.
// Reads the inode from disk if necessary.
void
uilock(struct inode *ip)
{
  struct buf *bp;
  struct v5dinode *v5dip;

  if(ip == 0 || ip->ref < 1)
    panic("uilock");

  acquiresleep(&ip->lock);

  if(ip->valid == 0){
    // bp = bread(ip->dev, 2);
    // v5dip = (struct v5dinode*)bp->data;

    //id nodes starts at sector 2, don't need to pass fs to UIBLOCK
    bp = bread(ip->dev, UIBLOCK(ip->inum));
    v5dip = (struct v5dinode*)bp->data + (ip->inum - 1) % UIPB;
    
    // cprintf("inum %d\n",ip->inum);
    // cprintf("uiblock %d\n",UIBLOCK(ip->inum));
    // cprintf("v5dip %x\n",(struct v5dinode*)bp->data + ip->inum - 1);
    // cprintf("i_mode %d i_nlink %d i_uid %d i_gid %d i_size0 %d i_size1 %d addr %d\n",v5dip->i_mode,v5dip->i_nlink,v5dip->i_uid, v5dip->i_gid, v5dip->i_size0, v5dip->i_size1, *(v5dip->i_addr));
    // cprintf("inum uilock: %d\n", ip->inum);

    if((v5dip->i_mode) & IFDIR){
      ip->type = 1;
    }else{
      ip->type = 2;
    }
    ip->major = 3;
    ip->minor = 1;
    ip->nlink = v5dip->i_nlink;
    ip->size = (v5dip->i_size0 << 16) + v5dip->i_size1;
    // cprintf("IP->SIZE: %d", ip->size);
    // uint cast_addr = v5dip->i_addr;

    // don't use memmove!!!
    // memmove(ip->addrs, v5dip->i_addr, sizeof(v5dip->i_addr));
    for(int i = 0; i < sizeof(ip->addrs)/sizeof(ip->addrs[0]); i++){
      if(i < sizeof(v5dip->i_addr)/sizeof(v5dip->i_addr[0])){
        ip->addrs[i] = v5dip->i_addr[i];
      }else{
        //fill it with 0
        ip->addrs[i] = 0;
      }
    }

    brelse(bp);
    ip->valid = 1;
    if(ip->type == 0){
      panic("uilock: no type");
    }
  }
}

// Unlock the given inode.
void
uiunlock(struct inode *ip)
{
  if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
    panic("uiunlock");

  releasesleep(&ip->lock);
}

// Drop a reference to an in-memory inode.
// If that was the last reference, the inode cache entry can
// be recycled.
// If that was the last reference and the inode has no links
// to it, free the inode (and its content) on disk.
// All calls to iput() must be inside a transaction in
// case it has to free the inode.
void
uiput(struct inode *ip)
{
  acquiresleep(&ip->lock);
  if(ip->valid && ip->nlink == 0){
    acquire(&uicache.lock);
    int r = ip->ref;
    release(&uicache.lock);
    if(r == 1){
      // inode has no links and no other references: truncate and free.
      itrunc(ip);
      ip->type = 0;
      uiupdate(ip);
      ip->valid = 0;
    }
  }
  releasesleep(&ip->lock);

  acquire(&uicache.lock);
  ip->ref--;
  release(&uicache.lock);
}

// Common idiom: unlock, then put.
void
uiunlockput(struct inode *ip)
{
  uiunlock(ip);
  uiput(ip);
}

//PAGEBREAK!
// Inode content
//
// The content (data) associated with each inode is stored
// in blocks on the disk. The first NDIRECT block numbers
// are listed in ip->addrs[].  The next NINDIRECT blocks are
// listed in block ip->addrs[NDIRECT].

// Return the disk block address of the nth block in inode ip.
// If there is no such block, bmap allocates one.
static ushort
bmap(struct inode *ip, uint bn)
{
  ushort addr, *a;
  struct buf *bp;

  if(bn < UNDIRECT){
    if((addr = ip->addrs[bn]) == 0){
      ip->addrs[bn] = addr = balloc(ip->dev);
      // cprintf("new addr: %d\n",ip->addrs[bn]);
    }
    return addr;
  }

  //from sys/ken/subr.c in the Unix v6 github repo (piazza)
  short i = bn>>8;
  if (bn & 0174000)
    i=7;
  if ((addr = ip->addrs[i]) == 0){
    ip->addrs[i] = addr = balloc(ip->dev);
    // cprintf("addr: %d\n",ip->addrs[bn]);
  }
  bp = bread(ip->dev, addr);
  a = (ushort*)bp->data;
  if ((addr = a[bn]) == 0) {
    a[bn] = addr = balloc(ip->dev);
  }
  brelse(bp);
  return addr;



  /*
  bn -= UNDIRECT;

  if(bn < UNINDIRECT){
    // Load indirect block, allocating if necessary.
    if((addr = ip->addrs[UNDIRECT]) == 0)
      ip->addrs[UNDIRECT] = addr = balloc(ip->dev);
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      a[bn] = addr = balloc(ip->dev);
      // log_write(bp);
      bwrite(bp);
    }
    brelse(bp);
    return addr;
  } */

  panic("bmap: out of range");
}

// Truncate inode (discard contents).
// Only called when the inode has no links
// to it (no directory entries referring to it)
// and has no in-memory reference to it (is
// not an open file or current directory).
static void
itrunc(struct inode *ip)
{

  int i, j;
  struct buf *bp;
  uint *a;

  for(i = 0; i < UNDIRECT; i++){
    if(ip->addrs[i]){
      bfree(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if(ip->addrs[UNDIRECT]){
    bp = bread(ip->dev, ip->addrs[UNDIRECT]);
    a = (uint*)bp->data;
    for(j = 0; j < UNINDIRECT; j++){
      if(a[j])
        bfree(ip->dev, a[j]);
    }
    brelse(bp);
    bfree(ip->dev, ip->addrs[UNDIRECT]);
    ip->addrs[UNDIRECT] = 0;
  }

  ip->size = 0;
  uiupdate(ip);
}

// Copy stat information from inode.
// Caller must hold ip->lock.
void
ustati(struct inode *ip, struct stat *st)
{
  st->dev = ip->dev;
  st->ino = ip->inum;
  st->type = ip->type;
  st->nlink = ip->nlink;
  st->size = ip->size;
}

//PAGEBREAK!
// Read data from inode.
// Caller must hold ip->lock.
int
ureadi(struct inode *ip, char *dst, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read){
      return -1;
    }
    return devsw[ip->major].read(ip, dst, n);
  }

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > ip->size)
    n = ip->size - off;

  for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
    bp = bread(ip->dev, bmap(ip, off/UBSIZE));
    m = min(n - tot, UBSIZE - off%UBSIZE);
    memmove(dst, bp->data + off%UBSIZE, m);
    brelse(bp);
  }
  return n;
}

// PAGEBREAK!
// Write data to inode.
// Caller must hold ip->lock.
int
uwritei(struct inode *ip, char *src, uint off, uint n)
{
  uint tot, m;
  struct buf *bp;

  if(ip->type == T_DEV){
    if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
      return -1;
    return devsw[ip->major].write(ip, src, n);
  }

  if(off > ip->size || off + n < off)
    return -1;
  if(off + n > UMAXFILE*UBSIZE)
    return -1;

  for(tot=0; tot<n; tot+=m, off+=m, src+=m){
    bp = bread(ip->dev, bmap(ip, off/UBSIZE));
    m = min(n - tot, UBSIZE - off%UBSIZE);
    memmove(bp->data + off%UBSIZE, src, m);
    // log_write(bp);
    bwrite(bp);
    brelse(bp);
  }

  if(n > 0 && off > ip->size){
    ip->size = off;
    uiupdate(ip);
  }
  return n;
}

//PAGEBREAK!
// Directories

int
unamecmp(const char *s, const char *t)
{
  return strncmp(s, t, UDIRSIZ);
}

// Look for a directory entry in a directory.
// If found, set *poff to byte offset of entry.
struct inode*
udirlookup(struct inode *dp, char *name, uint *poff)
{
  uint off, inum;
  struct udirent de;

  // cprintf("dp type: %d  T_DIR: %d\n", dp->type,T_DIR);

  if(dp->type != T_DIR)
    panic("udirlookup not DIR");

  for(off = 0; off < dp->size; off += sizeof(de)){
    if(ureadi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("udirlookup read");
    if(de.inum == 0)
      continue;
    // cprintf("de.name : %s\n",de.name);
    if(unamecmp(name, de.name) == 0){
      // entry matches path element
      if(poff)
        *poff = off;
      inum = de.inum;
      return iget(dp->dev, inum);
    }
  }

  return 0;
}

// Write a new directory entry (name, inum) into the directory dp.
int
udirlink(struct inode *dp, char *name, uint inum)
{
  int off;
  struct udirent de;
  struct inode *ip;

  // Check that name is not present.
  if((ip = udirlookup(dp, name, 0)) != 0){
    uiput(ip);
    return -1;
  }

  // Look for an empty dirent.
  for(off = 0; off < dp->size; off += sizeof(de)){
    if(ureadi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
      panic("dirlink read");
    if(de.inum == 0)
      break;
  }

  strncpy(de.name, name, UDIRSIZ);
  de.inum = inum;
  if(uwritei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
    panic("dirlink");

  return 0;
}

//PAGEBREAK!
// Paths

// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char*
skipelem(char *path, char *name)
{
  char *s;
  int len;

  while(*path == '/' || *path == '%')
    path++;
  if(*path == 0)
    return 0;
  s = path;
  while((*path != '/') && *path != 0)
    path++;
  len = path - s;
  if(len >= UDIRSIZ)
    memmove(name, s, UDIRSIZ);
  else {
    memmove(name, s, len);
    name[len] = 0;
  }
  while((*path == '/'))
    path++;
  return path;
}

// Look up and return the inode for a path name.
// If parent != 0, return the inode for the parent and copy the final
// path element into name, which must have room for DIRSIZ bytes.
// Must be called inside a transaction since it calls iput().
static struct inode*
namex(char *path, int unameiparent, char *name)
{
  struct inode *ip, *next;

  if(*path == '%'){
    ip = iget(XV5DEV, UROOTINO);
  }else{
    ip = uidup(myproc()->cwd);
  }

  while((path = skipelem(path, name)) != 0){
    uilock(ip);
    if(ip->type != T_DIR){
      uiunlockput(ip);
      return 0;
    }
    if(unameiparent && *path == '\0'){
      // Stop one level early.
      uiunlock(ip);
      return ip;
    }
    if((next = udirlookup(ip, name, 0)) == 0){
      uiunlockput(ip);
      return 0;
    }
    uiunlockput(ip);
    ip = next;
  }
  if(unameiparent){
    uiput(ip);
    return 0;
  }
  return ip;
}

struct inode*
unamei(char *path)
{
  char name[UDIRSIZ];
  return namex(path, 0, name);
}

struct inode*
unameiparent(char *path, char *name)
{
  return namex(path, 1, name);
}