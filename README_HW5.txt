

Part 2:

unix v5 is using disk 3 = DEV3.

FSSIZE in param.h increased to 4000 to support unix v5

The actual unix v5 superblock is stored in struct filsys. (IT DOESN'T HAVE "E")

Verify Inode

0002000 -> 140755 001411 000001 000240 006745 000000 000000 000000
0002020 -> 000000 000000 000000 000000 004720 070417 004615 063702
0002040 -> 140755 001402 000001 001660 000421 000652 000000 000000
0002060 -> 000000 000000 000000 000000 004720 067071 004471 021621
0002100 -> 100755 001401 000001 002752 000122 000123 000124 000000
0002120 -> 000000 000000 000000 000000 004720 065323 004471 021621

octal dump(first line):

0002000 -> 355 301 011 003 001 000 240 000 345 015 000 000 000 000

expected decimal value:
int     i_mode    ->  49645
char    i_nlink   ->  9
char    i_uid     ->  3
char    i_gid     ->  1
char    i_size0   ->  0
char    *i_size1  ->  160
int     i_addr[8] ->  3557

root -> 801100ac -> inum 1
bin -> 801100cc -> inum 2

each inum +32
dev -> inum = 60
expected dev addr = 8011080C
actual: 80110f20 ??????

derived calculation:

```bp = bread(ip->dev, UIBLOCK(ip->inum));
v5dip = (struct v5dinode*)bp->data + (ip->inum - 1) % UIPB;```

I have no idea why it works, but it works.


CD/READING/WRITING:

use % to access v5 root directory.

example:
cd %usr/sys/ken

supported commands:
cd/ls/cat/rm

Other notes:

The spaces and other characters in v5 files are displayed differently on the qemu terminal.
