#!/usr/bin/python

# todo: cache alignments for a given index (ie. lock alignment)

import sys
import re

if len(sys.argv) == 1:
    print "Enter structure size:"
    answer=sys.stdin.readline()
else:
    answer=sys.argv[1]

alignment=8
alignment=16
slabsize=4096
m=re.search(r'(\d+)(?:/(\d+))?', answer)
if not m:
    print "Invalid argument"
    sys.exit(1)
objsize=int(m.group(1))
if m.group(2):
    slabsize=int(m.group(2))
else:
    objsize=int(m.group(1))
objsize_unaligned=objsize
if objsize % 8 > 0:
    objsize+= 8 - objsize % 8
opers=int(slabsize/objsize)
slack=slabsize-opers*objsize
# todo: may change alignment!
perobjslack=slack/opers

print "Object size:     %d" %(objsize_unaligned)
print "O. size aligned: %d (align=%d, used for following calculations)" %(objsize, alignment)
print "Slab size:       %d" %(slabsize)
print "Objects/slab:    %d" %(opers)
print "Slack:           %d" %(slack)
print "Slack/object:    %.1f (how many bytes you can add to struct to keep objs/slab)" %(perobjslack)
