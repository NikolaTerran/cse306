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


// //from v5 system.h
// char	canonb[CANBSIZ];
// ushort	coremap[CMAPSIZ];
// ushort	swapmap[SMAPSIZ];
// ushort	*rootdir;
// ushort	execnt;
// ushort	lbolt;
// ushort	time[2];
// ushort	tout[2];
// struct	callo
// {
// 	ushort	c_time;
// 	ushort	c_arg;
// 	ushort	(*c_func)();
// } callout[NCALL];
// struct	mount
// {
// 	ushort	m_dev;
// 	ushort	*m_bufp;
// 	ushort	*m_inodp;
// } mount[NMOUNT];
// ushort	mpid;
// char	runin;
// char	runout;
// char	runrun;
// ushort	maxmem;
// ushort	*lks;
// ushort	rootdev;
// ushort	swapdev;
// ushort	swplo;
// ushort	nswap;
// ushort	updlock;
// ushort	rablock;


// //from v5 buf.h
// struct v5buf {
// 	ushort	b_flags;
// 	struct	buf *b_forw;
// 	struct	buf *b_back;
// 	struct	buf *av_forw;
// 	struct	buf *av_back;
// 	ushort	b_dev;
// 	ushort	b_wcount;
// 	char	*b_addr;
// 	char	*b_blkno;
// 	char	b_error;
// 	char	*b_resid;
// } buf[NBUF];

// /*
//  * forw and back are shared with "buf" struct.
//  */
// struct devtab {
// 	char	d_active;
// 	char	d_errcnt;
// 	struct	v5buf *b_forw;
// 	struct	v5buf *b_back;
// 	struct	v5buf *d_actf;
// 	struct 	v5buf *d_actl;
// };

// struct v5buf bfreelist;

// #define	B_WRITE	0
// #define	B_READ	01
// #define	B_DONE	02
// #define	B_ERROR	04
// #define	B_BUSY	010
// #define	B_XMEM	060
// #define	B_WANTED 0100
// #define	B_RELOC	0200
// #define	B_ASYNC	0400
// #define	B_DELWRI 01000


// //from v5 inode.h

// struct v5inode {
// 	char	i_flag;
// 	char	i_count;
// 	ushort	i_dev;
// 	ushort	i_number;
// 	ushort	i_mode;
// 	char	i_nlink;
// 	char	i_uid;
// 	char	i_gid;
// 	char	i_size0;
// 	char	*i_size1;
// 	ushort	i_addr[8];
// 	ushort	i_lastr;
// } inode[NINODE];

/* flags */
#define	ILOCK	01
#define	IUPD	02
#define	IACC	04
#define	IMOUNT	010
#define	IWANT	020
#define	ITEXT	040

/* modes */
#define	IALLOC	0100000
#define	IFMT	060000
#define		IFDIR	040000
#define		IFCHR	020000
#define		IFBLK	060000
#define	ILARG	010000
#define	ISUID	04000
#define	ISGID	02000
#define ISVTX	01000
#define	IREAD	0400
#define	IWRITE	0200
#define	IEXEC	0100


// //v5 user.h

// struct user {
// 	ushort	u_rsav[2];		/* must be first */
// 	ushort	u_fsav[25];		/* must be second */
// 	char	u_segflg;
// 	char	u_error;
// 	char	u_uid;
// 	char	u_gid;
// 	char	u_ruid;
// 	char	u_rgid;
// 	ushort	u_procp;
// 	char	*u_base;
// 	char	*u_count;
// 	char	*u_offset[2];
// 	ushort	*u_cdir;
// 	char	u_dbuf[DIRSIZ];
// 	char	*u_dirp;
// 	struct	{
// 		ushort	u_ino;
// 		char	u_name[DIRSIZ];
// 	} u_dent;
// 	ushort	*u_pdir;
// 	ushort	u_uisa[8];
// 	ushort	u_uisd[8];
// 	ushort	u_ofile[NOFILE];
// 	ushort	u_arg[5];
// 	ushort	u_tsize;
// 	ushort	u_dsize;
// 	ushort	u_ssize;
// 	ushort	u_qsav[2];
// 	ushort	u_ssav[2];
// 	ushort	u_signal[NSIG];
// 	ushort	u_utime;
// 	ushort	u_stime;
// 	ushort	u_cutime[2];
// 	ushort	u_cstime[2];
// 	ushort	*u_ar0;
// 	ushort	u_prof[4];
// 	char	u_nice;
// 	char	u_dsleep;
// } u;	/* u = 140000 */

// /* u_error codes */
// #define	EFAULT	106
// #define	EPERM	1
// #define	ENOENT	2
// #define	ESRCH	3
// #define	EIO	5
// #define	ENXIO	6
// #define	E2BIG	7
// #define	ENOEXEC	8
// #define	EBADF	9
// #define	ECHILD	10
// #define	EAGAIN	11
// #define	ENOMEM	12
// #define	EACCES	13
// #define	ENOTBLK	15
// #define	EBUSY	16
// #define	EEXIST	17
// #define	EXDEV	18
// #define	ENODEV	19
// #define	ENOTDIR	20
// #define	EISDIR	21
// #define	EINVAL	22
// #define	ENFILE	23
// #define	EMFILE	24
// #define	ENOTTY	25
// #define	ETXTBSY	26
// #define	EFBIG	27
// #define	ENOSPC	28
// #define	ESPIPE	29
// #define	EROFS	30
// #define	EMLINK	31