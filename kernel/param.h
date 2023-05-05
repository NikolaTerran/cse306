#define NPROC        64  // maximum number of processes
#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NOFILE       16  // open files per process
#define NFILE       100  // open files per system
// #define NINODE       50  // maximum number of active i-nodes // use v5 value instead
#define NDEV         10  // maximum major device number
#define ROOTDEV       1  // device number of file system root disk
#define DEV2          2
#define DEV3          3
#define XV5DEV        3
#define MAXARG       32  // max exec arguments
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       4000  // size of file system in blocks


//copied from v5 src code

/*
 * variables
 */

#define	NBUF	(MAXOPBLOCKS*3)
#define	NINODE	100
#define	NMOUNT	5
#define	NEXEC	4
#define	MAXMEM	(32*32)
#define	SSIZE	20
#define	SINCR	20
#define	CANBSIZ	256
#define	CMAPSIZ	100
#define	SMAPSIZ	100
#define	NCALL	20
#define	NTEXT	20
#define	NCLIST	100

/*
 * priorities
 * probably should not be
 * altered too much
 */

#define	PSWP	-100
#define	PINOD	-90
#define	PRIBIO	-50
#define	PPIPE	1
#define	PWAIT	40
#define	PSLEP	90
#define	PUSER	100

/*
 * signals
 * dont change
 */

#define	NSIG	13
#define		SIGHUP	1
#define		SIGINT	2
#define		SIGQIT	3
#define		SIGINS	4
#define		SIGTRC	5
#define		SIGIOT	6
#define		SIGEMT	7
#define		SIGFPT	8
#define		SIGKIL	9
#define		SIGBUS	10
#define		SIGSEG	11
#define		SIGSYS	12

/*
 * fundamental constants
 * cannot be changed
 */

#define	USIZE	16
// #define	NULL	0
#define	NODEV	(-1)
#define	ROOTINO	1
#define	DIRSIZ	14

