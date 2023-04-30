

Part 2:

unix v5 is using disk 3.

usuperblock is the xv6 superblock but I added a u in the front.
The actual unix v5 superblock is stored in struct filsys. (IT DOESN'T HAVE "E")

THings that maybe need to be changed in the future:
line 10 buf.h, BSIZE -> UBSIZE
line 25 file.h NDIRECT -> 12


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