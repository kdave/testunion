/*
 * Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Does time test of sync for creating, reading and deleting files.
 *
 * To compile:
 *   cc syncperf.c -rlt -o syncperf
 *
 * To run, cd to a directory on volume where test should run.
 *   syncperf
 *
 * The syncperf will create the directory "synctestdir" and do
 * several test runs creating twice as many files each time.
 *
 * The test prints the time to sync after creating the files,
 * after reading the files and after deleting the files.
 *
 * There will be some runs, where the read sync time is
 * fast event on systems that exhibit the problem.
 *
 * When the tests finishes, it cleans up "synctestdir".
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

enum {	MAX_NAME = 12,
	FILE_SIZE = 1 << 14,
	BYTES_TO_READ = 1,
	NUM_START_FILES = 1 << 15,
	NUM_FILES = 1 << 15 };

struct file {
	char name[MAX_NAME];
};

struct file *File;

static inline uint64_t nsecs(void)
{
	struct timespec	t;

	clock_gettime(CLOCK_REALTIME, &t);
	return (uint64_t)t.tv_sec * 1000000000ULL + t.tv_nsec;
}

static void fill(uint8_t *buf, int n)
{
	int i;

	for (i = 0; i < n; i++)
		buf[i] = rand();
}

static void createfiles(int num_files)
{
	uint8_t buf[FILE_SIZE];
	int fd;
	int i;

	fill(buf, sizeof(buf));
	for (i = 0; i < num_files; i++) {
		snprintf(File[i].name, MAX_NAME, "f%d", i);
		fd = creat(File[i].name, 0600);
		if (write(fd, buf, sizeof(buf)) == -1)
			perror("write");
		close(fd);
	}
}

static void unlinkfiles(int num_files)
{
	int i;

	for (i = 0; i < num_files; i++)
		unlink(File[i].name);
}

static void readfiles(int num_files)
{
	uint8_t buf[BYTES_TO_READ];
	int fd;
	int i;

	for (i = 0; i < num_files; i++) {
		fd = open(File[i].name, O_RDONLY);
		if (read(fd, buf, BYTES_TO_READ) == -1)
			perror("read");
		close(fd);
	}
}

static void time_sync(const char *label, int n)
{
	uint64_t start;
	uint64_t finish;

	start = nsecs();
	sync();
	finish = nsecs();
	printf("%10s %8d. %10.2f ms\n",
		label, n, (double)(finish - start)/1000000.0);
}

void crsyncdel(int n)
{
	createfiles(n);
	time_sync("create", n);
	readfiles(n);
	time_sync("read", n);
	unlinkfiles(n);
	time_sync("unlink", n);
}

static void cleanup(const char *dir)
{
	char	cmd[1024];

	if (chdir("..") == -1)
		perror(dir);
	snprintf(cmd, sizeof(cmd), "rm -fr %s", dir);
	if (system(cmd) == -1)
		perror(cmd);
}

static void setup(const char *dir)
{
	mkdir(dir, 0700);
	if (chdir(dir) == -1)
		perror(dir);
	sync();
	File = malloc(sizeof(*File) * NUM_FILES);
}

int main(int argc, char *argv[])
{
	char *dir = "synctestdir";
	int i;

	setup(dir);
	/*
	 * Number of files grows by powers of two until the
	 * specified number of files is reached.
	 * Start with a large enough number to skip noise.
	 */
	for (i = NUM_START_FILES; i <= NUM_FILES; i <<= 1)
		crsyncdel(i);
	cleanup(dir);
	return 0;
}
