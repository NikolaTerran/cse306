

Part 2:

unix v5 is using disk 3.

usuperblock is the xv6 superblock but I added a u in the front.
The actual unix v5 superblock is stored in struct filsys. (IT DOESN'T HAVE "E")

THings that maybe need to be changed in the future:
line 10 buf.h, BSIZE -> UBSIZE
line 25 file.h NDIRECT -> 12
