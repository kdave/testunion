#if 0
g++ -Iinclude -o snaptest snaptest.cc libleveldb.a -pthread

> Quoting Chris Mason (2013-03-21 14:06:14)
>> With mmap the kernel can pick any given time to start writing out dirty
>> pages.  The idea is that if the application makes more changes the page
>> becomes dirty again and the kernel writes it again.

That's the theory.  But what if there's some race between the time the
page is frozen for compressing and the time it's marked as clean, or
it's marked as clean after it's further modified, or a subsequent write
to the same page ends up overridden by the background compression of the
old contents of the page?  These are all possibilities that come to mind
without knowing much about btrfs inner workings.

>> So the question is, can you trigger this without snapshots being done
>> at all?

I haven't tried, but I now have a program that hit the error condition
while taking snapshots in background with small time perturbations to
increase the likelihood of hitting a race condition at the exact time.
It uses leveldb's infrastructure for the mmapping, but it shouldn't be
too hard to adapt it so that it doesn't.

> So my test program creates an 8GB file in chunks of 1MB each.

That's probably too large a chunk to write at a time.  The bug is
exercised with writes slightly smaller than a single page (although
straddling across two consecutive pages).

This half-baked test program (hereby provided under the terms of the GNU
GPLv3+) creates a btrfs subvolume and two files in it: one in which I/O
will be performed with write()s, another that will get the same data
appended with leveldb's mmap-based output interface.  Random block
sizes, as well as milli and microsecond timing perturbations, are read
from /dev/urandom, and the rest of the output buffer is filled with
(char)1.

The test that actually failed (on the first try!, after some other
variations that didn't fail) didn't have any of the #ifdef options
enabled (i.e., no -D* flags during compilation), but it triggered the
exact failure observed with ceph: zeros at the end of a page where there
should have been nonzero data, followed by nonzero data on the following
page!  That was within snapshots, not in the main subvol, but hopefully
it's the same problem, just a bit harder to trigger.

I can't tell whether memory pressure is required to hit the problem.
The system on which I hit the error was mostly otherwise idle while
running the test, but starting so many shell commands in background
surely creates intense activity on the system, possibly increasing the
odds that some race condition will hit.

Two subsequent runs of the program failed to trigger the problem.

#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <sstream>
#include "leveldb/env.h"

#ifndef MAXROUNDS
#define MAXROUNDS 400
#endif

int main () {
  leveldb::Env *env = leveldb::Env::Default();
  leveldb::Status s;
  std::string str;
  int wd;
  int rd;
  leveldb::WritableFile *out;
  char lenbuf[1];
  char buf[4096];
  unsigned long long totalsize = 0;
  int blocks;
  pid_t __attribute__((__unused__)) pid = getpid();

  memset(buf, 1, sizeof(buf));

  rd = open("/dev/urandom", O_RDONLY);
  if (rd == -1) {
    perror ("open random");
    abort ();
  }

  str = "btrfs su cr snaptest.";
  if (system (str.c_str())) {
    perror ("subvol create");
    abort ();
  }

  str = "snaptest./";
  unlink((str + "ca").c_str ());
  wd = open((str + "ca").c_str (), O_CREAT | O_TRUNC | O_WRONLY, 0644);
  if (wd == -1) {
    perror ("open wd");
    abort ();
  }
  
  unlink((str + "db").c_str ());
  s = env->NewWritableFile(str + "db", &out);
  if (!s.ok()) {
    perror ("open db");
    abort ();
  }

  for (blocks = 0; blocks < MAXROUNDS; blocks++) {
    if (read (rd, buf, 3) != 3) {
      printf ("\nread error: %s\n", strerror (errno));
      break;
    }

    printf("\r%i blocks, %llu total size\n",
	   blocks, totalsize);

#if !NOBGCMP
    std::ostringstream os;
    if (buf[1] || buf[2])
      os << "usleep " << 1000L * (long)(unsigned char)buf[1]
	+ (buf[1] ? (long)(signed char)buf[2] : (unsigned char)buf[2])
	 << " && ";
#if !NOSNAPS
    os << "btrfs su snap snaptest. snaptest." << blocks
       << " && sleep 5 && if cmp -n `stat -c %s snaptest." << blocks
       << "/ca` snaptest." << blocks
       << "/??; then btrfs su del snaptest." << blocks
       << "; else kill " << pid << "; fi &";
#else
    os << "sleep 5 && if cmp -n " << totalsize << " snaptest." << blocks
       << "/??; then :; else kill " << pid << "; fi &";
#endif
    if (system (os.str().c_str())) {
      printf ("\nsnap error: %s\n", strerror (errno));
      break;
    }
#endif

    int size = 4096 - (unsigned char)buf[0];

    s = out->Append(leveldb::Slice(buf, size));
    if (!s.ok ()) {
      printf("\nappend error: %s\n", s.ToString().c_str());
      break;
    }

    if (write (wd, buf, size) != size) {
      printf("\nwrite error: %s\n", strerror (errno));
      break;
    }

    if (buf[1])
      for (timespec tv = { 0, (unsigned char)buf[1] * 1000000L };
	   nanosleep(&tv, &tv);)
	;

    totalsize += size;
  }

  printf("\r%i blocks, %llu total size\n",
	 blocks, totalsize);

#if NOBGCMP
  if (system("cmp snaptest./??")) {
    printf ("\ncmp error: %s\n", strerror (errno));
    break;
  }
#endif
}
