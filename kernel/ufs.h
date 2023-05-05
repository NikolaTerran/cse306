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

// //this is not used, only serve as comparison tool
// struct usuperblock {
//   uint size;         // Size of file system image (blocks)
//   uint nblocks;      // Number of data blocks
//   uint ninodes;      // Number of inodes.
//   uint nlog;         // Number of log blocks
//   uint logstart;     // Block number of first log block
//   uint inodestart;   // Block number of first inode block
//   uint bmapstart;    // Block number of first free map block
// };

#define NUM_INODES 100
//v5 superblock code but i changed it to use correct type
struct  filsys {
  ushort s_isize;      // number of sectors of inode
  ushort s_fsize;    // number of disk sectors
  ushort s_nfree;    //number of valid entries in s_free array
  ushort s_free[100];   // holds sector numbers of free sectors
  ushort s_ninode;   //number of valid entries in the s_inode array
  ushort  s_inode[100];  //sector numbers of free inodes
  uchar    s_flock;  // ?
  uchar    s_ilock;  // ?
  uchar    s_fmod;  // ?
  uchar    s_ronly; // ?
  ushort     s_time[2]; // unix time?
};

#define UNDIRECT 8
#define UNINDIRECT 8
#define UMAXFILE (UNDIRECT + UNINDIRECT)

// // On-disk inode structure (NOT USED)
// struct udinode {
//   short type;           // File type
//   short major;          // Major device number (T_DEV only)
//   short minor;          // Minor device number (T_DEV only)
//   short nlink;          // Number of links to inode in file system
//   uint size;            // Size of file (bytes)
//   uint addrs[UNDIRECT+1];   // Data block addresses
// };

// //I don't think we are going to use this
// struct v5inode {
//         uchar    i_flag;
//         uchar    i_count;
//         ushort     i_dev;
//         ushort     i_number;
//         ushort     i_mode;
//         uchar    i_nlink;
//         uchar    i_uid;
//         uchar    i_gid;
//         uchar    i_size0;
//         unsigned char* i_size1;
//         ushort     i_addr[8];
//         unsigned long long garbage;
// };

// on-disk inode structure
struct v5dinode {
        ushort   i_mode;
        uchar    i_nlink;
        uchar    i_uid;
        uchar    i_gid;
        uchar    i_size0;
        ushort   i_size1;       // cast to char *
        ushort   i_addr[8];        // cast to ushort array
        unsigned long long garbage;
};

/* flags */
#define ILOCK   01
#define IUPD    02
#define IACC    04
#define IMOUNT  010
#define IWANT   020
#define ITEXT   040

/* modes */
#define IALLOC  0100000
#define IFMT    060000
#define         IFDIR   040000
#define         IFCHR   020000
#define         IFBLK   060000
#define ILARG   010000
#define ISUID   04000
#define ISGID   02000
#define ISVTX   01000
#define IREAD   0400
#define IWRITE  0200
#define IEXEC   0100


// Inodes per block.
#define UIPB           (UBSIZE / sizeof(struct v5dinode))

// Block containing inode i
#define UIBLOCK(i)     ((i) / UIPB + 2UL)

// Bitmap bits per block
#define UBPB           (UBSIZE*8)

// Block of free map containing bit for block b
#define UBBLOCK(b, sb) (b/UBPB + 1)//sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define UDIRSIZ 14

struct udirent {
  ushort inum;
  char name[UDIRSIZ];
};
