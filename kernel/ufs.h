// On-disk file system format.
// Both the kernel and user programs use this header file.


#define UROOTINO 1  // root i-number
#define UBSIZE 512  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct usuperblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

//v5 code but i changed it to use correct type
struct  filsys {
  ushort s_isize;      // number of sectors of inode
  ushort s_fsize;    // number of disk sectors
  ushort s_nfree;    //number of valid entries in s_free array
  ushort s_ninode;   //number of valid entries in the s_inode array
  ushort  s_free[100];  //sector numbers of free sector
  char    s_flock;  // ?
  char    s_ilock;  // ?
  char    s_fmod;  // ?
  char    s_ronly; // ?
  ushort     s_time[2]; // unix time?
};

#define UNDIRECT 12
#define UNINDIRECT (UBSIZE / sizeof(uint))
#define UMAXFILE (UNDIRECT + UNINDIRECT)

// On-disk inode structure
struct udinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[UNDIRECT+1];   // Data block addresses
};





// Inodes per block.
#define UIPB           (UBSIZE / sizeof(struct udinode))

// Block containing inode i
#define UIBLOCK(i, sb)     ((i) / UIPB + sb.inodestart)

// Bitmap bits per block
#define UBPB           (UBSIZE*8)

// Block of free map containing bit for block b
#define UBBLOCK(b, sb) (b/UBPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define UDIRSIZ 14

struct udirent {
  ushort inum;
  char name[UDIRSIZ];
};
