#if 0
From: Andy Lutomirski <luto@amacapital.net>
Date: 	Mon, 20 Jan 2014 21:30:24 -0800
Message-ID: <CALCETrVLF_1d5cg5izPLmZa_e7bxHH-rrgp-m0uGFmuVwO_73w@mail.gmail.com>
Subject: [BTRFS-specific] Re: Dirty deleted files cause pointless I/O storms (unless truncated first)

[cc: btrfs]

On Mon, Jan 20, 2014 at 8:46 PM, Dave Chinner <david@fromorbit.com> wrote:
> On Mon, Jan 20, 2014 at 04:59:23PM -0800, Andy Lutomirski wrote:
>> The code below runs quickly for a few iterations, and then it slows
>> down and the whole system becomes laggy for far too long.
>>
>> Removing the sync_file_range call results in no I/O being performed at
>> all (which means that the kernel isn_t totally screwing this up), and
>> changing "4096" to SIZE causes lots of I/O but without
>> the going-out-to-lunch bit (unsurprisingly).
>
> More details please. hardware, storage, kernel version, etc.

The kernel is 3.11.10-301.fc20.x86_64.  It_s an excessively fast CPU
(Intel i7-3930K) with 16GB RAM and a Corsair Force 3 SSD (6Gb/s SATA)
SSD.  The FS is btrfs on LVM on dm-crypt.

In that setup, this thing goes quickly for 100 iterations or so, at
which point even trying to Ctrl-C it lags out for ten seconds or so.

I clearly should have tested more thoroughly, though -- I can_t
reproduce this problem on ext4.

>
> I can_t reproduce any slowdown with the code as posted on a VM
> running 3.31-rc5 with 16GB RAM and an SSD w/ ext4 or XFS. The
> workload is only generating about 80 IOPS on ext4 so even a slow
> spindle should be able handle this without problems...
>
>> Surprisingly, uncommenting the ftruncate call seems to fix the
>> problem.  This suggests that all the necessary infrastructure to avoid
>> wasting time writing to deleted files is there but that it_s not
>> getting used.
>
> Not surprising at all - if it_s stuck in a writeback loop somewhere,
> truncating the file will terminate writeback because it end up being
> past EOF and so stops immediately...

Presumably ext4 and xfs are smart enough to stop writeback when the
inode is gone, but btrfs is still either keeping the inode alive or
just finishes writeback anyway.

--Andy
#endif

#define _GNU_SOURCE
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SIZE (16 * 1048576)

static void hammer(const char *name)
{
  int fd = open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
  if (fd == -1)
    err(1, "open");

  fallocate(fd, 0, 0, SIZE);

  void *addr = mmap(NULL, SIZE, PROT_WRITE, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED)
    err(1, "mmap");

  memset(addr, 0, SIZE);

  if (munmap(addr, SIZE) != 0)
    err(1, "munmap");

  if (sync_file_range(fd, 0, 4096,
              SYNC_FILE_RANGE_WAIT_BEFORE | SYNC_FILE_RANGE_WRITE |
              SYNC_FILE_RANGE_WAIT_AFTER) != 0)
    err(1, "sync_file_range");

  if (unlink(name) != 0)
    err(1, "unlink");

#if 1
  if (ftruncate(fd, 0) != 0)
     err(1, "ftruncate");
#endif

  close(fd);
}

int main(int argc, char **argv)
{
  if (argc != 2) {
    printf("Usage: hammer_and_delete FILENAME\n");
    return 1;
  }

  while (true) {
    hammer(argv[1]);
    write(1, ".", 1);
  }
}
