#!/usr/bin/python
# author/source: unknown, internet

from contextlib import contextmanager
import struct
import fcntl
import sys
import os
import array

# context friendly os.open (normal open() doesn't work on dirs)
@contextmanager
def osopen(file, mode=os.O_RDONLY):
    try:
        fd = os.open(file, mode)
        yield fd
    finally:
        os.close(fd)

def sizeof(x):
    return struct.calcsize(x)

IOCPARM_MASK = 0x7f
IOC_OUT = 0x40000000
IOC_IN = 0x80000000
IOC_INOUT = (IOC_IN|IOC_OUT)

# defines from LINUX_FIEMAP_H
#define FIEMAP_MAX_OFFSET   (~0ULL)
#define FIEMAP_FLAG_SYNC    0x00000001 /* sync file data before map */
#define FIEMAP_FLAG_XATTR   0x00000002 /* map extended attribute tree */
#define FIEMAP_FLAGS_COMPAT (FIEMAP_FLAG_SYNC | FIEMAP_FLAG_XATTR)
FIEMAP_EXTENT_LAST          = 0x00000001 # Last extent in file. */
FIEMAP_EXTENT_UNKNOWN       = 0x00000002 # Data location unknown. */
FIEMAP_EXTENT_DELALLOC      = 0x00000004 # Location still pending.
#                            * Sets EXTENT_UNKNOWN. */
FIEMAP_EXTENT_ENCODED       = 0x00000008 # Data can not be read
#                            * while fs is unmounted */
FIEMAP_EXTENT_DATA_ENCRYPTED = 0x00000080 # Data is encrypted by fs.
#                            * Sets EXTENT_NO_BYPASS. */
FIEMAP_EXTENT_NOT_ALIGNED   = 0x00000100 # Extent offsets may not be
#                            * block aligned. */
FIEMAP_EXTENT_DATA_INLINE   = 0x00000200 # Data mixed with metadata.
#                            * Sets EXTENT_NOT_ALIGNED.*/
FIEMAP_EXTENT_DATA_TAIL     = 0x00000400 # Multiple files in block.
#                            * Sets EXTENT_NOT_ALIGNED.*/
FIEMAP_EXTENT_UNWRITTEN     = 0x00000800 # Space allocated, but
#                            * no data (i.e. zero). */
FIEMAP_EXTENT_MERGED        = 0x00001000 # File does not natively
#                            * support extents. Result
#                            * merged for efficiency. */
#define FIEMAP_EXTENT_SHARED        0x00002000 /* Space shared with other
#                            * files. */

_flags = {}
_flags[FIEMAP_EXTENT_UNKNOWN] = "unknown"
_flags[FIEMAP_EXTENT_DELALLOC] = "delalloc"
_flags[FIEMAP_EXTENT_DATA_ENCRYPTED] = "encrypted"
_flags[FIEMAP_EXTENT_NOT_ALIGNED] = "not_aligned"
_flags[FIEMAP_EXTENT_DATA_INLINE] = "inline"
_flags[FIEMAP_EXTENT_DATA_TAIL] = "tail_packed"
_flags[FIEMAP_EXTENT_UNWRITTEN] = "unwritten"
_flags[FIEMAP_EXTENT_MERGED] = "merged"
_flags[FIEMAP_EXTENT_LAST] = "eof"

def _IOWR(x, y, t):
    return (IOC_INOUT|((sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|y)

struct_fiemap = '=QQLLLL'
struct_fiemap_extent = '=QQQQQLLLL'

sf = sizeof(struct_fiemap)
sfe = sizeof(struct_fiemap_extent)

FS_IOC_FIEMAP = _IOWR (ord('f'), 11, struct_fiemap)

# shift is for reporting in blocks instead of bytes
shift = 0#12

def parse_fiemap_extents(string, num):
    '''return dict of fiemap_extents struct values'''
    ex = []
    for e in range(num):
        i = e*sfe 
        x = [x >> shift for x in struct.unpack(struct_fiemap_extent, string[i:i+sfe])]
        flags = ' '.join(_flags[z] for z in _flags.keys() if (x[5]&z>0))
        ex.append({'logical':x[0],'physical':x[1],'length':x[2],'flags':flags})
    return ex

def parse_fiemap(string):
    '''return dict of fiemap struct values'''
    # split fiemap struct
    res = struct.unpack(struct_fiemap, string[:sf])
    return {'start':res[0], 'length':res[1], 'flags':res[2], 'mapped_extents':res[3],
            'extent_count':res[4], 'extents':parse_fiemap_extents(string[sf:], res[4])}

def fiemap_ioctl(fd, num_ext=0):
    # build fiemap struct
    buf = struct.pack(struct_fiemap , 0, 0xffffffffffffffff, 0, 0, num_ext, 0)
    # add room for fiemap_extent struct array
    buf += '\0'*num_ext*sfe
    # use a mutable buffer to get around ioctl size limit
    buf = array.array('c', buf)
    # ioctl call
    ret = fcntl.ioctl(fd, FS_IOC_FIEMAP, buf)
    return buf.tostring()

def fiemap(file=None, fd=None, get_extents=True):
    if fd is None and file is None:
        raise TypeError('must provide either a filename or file descriptor')

    def _do(fd):
        # first call to get number of extents
        res = fiemap_ioctl(fd)
        # second call to get extent info
        if get_extents:
            res = fiemap_ioctl(fd, parse_fiemap(res)['mapped_extents'])
        return parse_fiemap(res), res

    if fd is None:
        with osopen(file) as fd:
            res = _do(fd)
    else:
        res = _do(fd)

    return res

if __name__ == '__main__':
    import json
    file = len(sys.argv) == 2 and sys.argv[1] or '.'
    print json.dumps(fiemap(file)[0], indent=2)

