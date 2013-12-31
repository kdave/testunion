#!/usr/bin/python

import os
from stat import *
import fcntl
import array

names = os.listdir('.')
lastino = 0
name_to_ino_in = 0
name_to_ino_out = 0
lastblock = 0
name_to_block_in = 0
name_to_block_out = 0
iblocks = list()
inode_to_block_in = 0
inode_to_block_out = 0

for file in names :
    try :
        st = os.stat(file)
    except OSError:
        continue
    if not S_ISREG(st.st_mode) :
        continue
    if st.st_ino > lastino :
        name_to_ino_in += 1
    else : name_to_ino_out += 1
    lastino = st.st_ino
    f = open(file)
    buf = array.array('I', [0])
    err = fcntl.ioctl(f.fileno(), 1, buf)
    if err != 0 :
        print "ioctl failed on " + f
    block = buf[0]
    if block != 0 :
        if block > lastblock :
            name_to_block_in += 1
        else : name_to_block_out += 1
        lastblock = block
        iblocks.append((st.st_ino,block))
print "Name to inode correlation: " + str(float(name_to_ino_in) / float((name_to_ino_in + name_to_ino_out)))
print "Name to block correlation: " + str(float(name_to_block_in) / float((name_to_block_in + name_to_block_out)))
iblocks.sort()
lastblock = 0
for i in iblocks:
    if i[1] > lastblock:
        inode_to_block_in += 1
    else: inode_to_block_out += 1
    lastblock = i[1]
print "Inode to block correlation: " + str(float(inode_to_block_in) / float((inode_to_block_in + inode_to_block_out)))
