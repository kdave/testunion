/*
 * Write-After-Checksum reproducer(?) program
 * Copyright (C) 2011 IBM.  All rights reserved.
 * This program is licensed under the GPLv2.
 */
#define _XOPEN_SOURCE 600
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h>
#define __USE_GNU
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

#define SYNC_RANGE	1
#define SYNC_FILE	2

#define DEFAULT_BUFSIZE	4096
static uint32_t bufsize = DEFAULT_BUFSIZE;

void help(const char *pname)
{
	printf("Usage: %s [-n threads] [-m threads] [-d] [-b blocksize] [-r] [-f] -l filesize filename\n", pname);
	printf("-b	Size of a memory page.\n");
	printf("-d	Use direct I/O.\n");
	printf("-l	Desired file size.\n");
	printf("-n	Use this many write() threads.\n");
	printf("-m	Use this many mmap write threads.\n");
	printf("-s	Synchronous disk writes.\n");
	printf("-r	Use sync_file_range after write.\n");
	printf("-f	fsync after write.\n");
}

int seed_random(void) {
	int fp;
	long seed;

	fp = open("/dev/urandom", O_RDONLY);
	if (fp < 0) {
		perror("/dev/urandom");
		return 0;
	}

	if (read(fp, &seed, sizeof(seed)) != sizeof(seed)) {
		perror("read random seed");
		return 0;
	}

	close(fp);
	srand(seed);

	return 1;
}

uint64_t get_randnum(uint64_t min, uint64_t max) {
	return (min + (uint64_t)((double)(max - min) * (rand() / (RAND_MAX + 1.0))));
}

static uint64_t get_randnum_align(uint64_t min, uint64_t max, uint64_t align) {
	return (min + (uint64_t)((double)(max - min) * (rand() / (RAND_MAX + 1.0)))) &
		~(align - 1);
}

int write_junk(const char *fname, int flags, int sync_options, uint64_t file_size)
{
	int fd, len;
	uint64_t offset, generation = 0;
	char *buf;

	len = posix_memalign((void **)&buf, bufsize, bufsize);
	if (len) {
		errno = len;
		perror("alloc");
		return 66;
	}

	fd = open(fname, flags | O_WRONLY);
	if (fd < 1) {
		perror(fname);
		return 64;
	}

	while (1) {
		len = snprintf(buf, bufsize - 1, "%d - %"PRIu64, getpid(), generation++);
		if (flags & O_DIRECT) {
			len = bufsize;
			offset = get_randnum_align(0, file_size - len, bufsize);
		} else {
			offset = get_randnum(0, file_size - len);
		}

		if (pwrite(fd, buf, len, offset) < 0) {
			perror("pwrite");
			close(fd);
			free(buf);
			return 65;
		}
		if ((sync_options & SYNC_RANGE) && sync_file_range(fd, offset, len, SYNC_FILE_RANGE_WAIT_BEFORE |
SYNC_FILE_RANGE_WRITE | SYNC_FILE_RANGE_WAIT_AFTER) < 0) {
			perror("sync_file_range");
			close(fd);
			free(buf);
			return 67;
		}
		if ((sync_options & SYNC_FILE) && fsync(fd)) {
			perror("fsync");
			close(fd);
			free(buf);
			return 68;
		}
	}

	return 0;
}

int mmap_junk(const char *fname, int flags, int sync_options, uint64_t file_size)
{
	int fd, len;
	uint64_t offset, generation = 0;
	char *buf, *map;
	long page_size;

	page_size = sysconf(_SC_PAGESIZE);
	if (page_size < 0) {
		perror("_SC_PAGESIZE");
		return 101;
	}

	fd = open(fname, flags | O_RDWR);
	if (fd < 1) {
		perror(fname);
		return 96;
	}

	len = posix_memalign((void **)&buf, bufsize, bufsize);
	if (len) {
		errno = len;
		perror("alloc");
		return 102;
	}

	map = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		perror(fname);
		return 97;
	}

	while (1) {
		len = snprintf(buf, bufsize - 1, "%d - %"PRIu64, getpid(), generation++);
		if (flags & O_DIRECT) {
			len = bufsize;
			offset = get_randnum_align(0, file_size - len, bufsize);
		} else {
			offset = get_randnum(0, file_size - len);
		}

		memcpy(map + offset, buf, len);
		len += offset & (page_size - 1);
		offset &= ~(page_size - 1);
		if ((sync_options & SYNC_RANGE) && msync(map + offset, len, MS_SYNC | MS_INVALIDATE)) {
			perror("msync");
			munmap(map, file_size);
			close(fd);
			free(buf);
			return 99;
		}
		if ((sync_options & SYNC_FILE) && fsync(fd)) {
			perror("fsync");
			munmap(map, file_size);
			close(fd);
			free(buf);
			return 100;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int opt, fd;
	unsigned long i, mthreads = 0, nthreads = 1;
	char *fname = NULL;
	int flags = 0, sync_options = 0;
	uint64_t file_size = 0;
	pid_t pid;
	int status;

	while ((opt = getopt(argc, argv, "b:dn:l:srfm:")) != -1) {
		switch (opt) {
		case 'd':
			flags |= O_DIRECT;
			break;
		case 'n':
			nthreads = strtoul(optarg, NULL, 0);
			break;
		case 'm':
			mthreads = strtoul(optarg, NULL, 0);
			break;
		case 'l':
			file_size = strtoull(optarg, NULL, 0);
			break;
		case 's':
			flags |= O_SYNC;
			break;
		case 'b':
			bufsize = strtoul(optarg, NULL, 0);
			break;
		case 'r':
			sync_options |= SYNC_RANGE;
			break;
		case 'f':
			sync_options |= SYNC_FILE;
			break;
		default:
			help(argv[0]);
			return 4;
		}
	}

	if (optind != argc - 1 || file_size < 1) {
		help(argv[0]);
		return 1;
	}

	fname = argv[optind];

	if (!seed_random())
		return 2;

	// truncate first
	fd = open(fname, flags | O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		perror(fname);
		return 3;
	}
	status = posix_fallocate(fd, 0, file_size);
#if 0
	if (status) {
		perror(fname);
		return 4;
	}
#endif
	close(fd);

	// spawn threads and go to town
	if (nthreads == 1)
		return write_junk(fname, flags, sync_options, file_size);

	for (i = 0; i < nthreads; i++) {
		pid = fork();
		if (!pid)
			return write_junk(fname, flags, sync_options, file_size);
	}

	for (i = 0; i < mthreads; i++) {
		pid = fork();
		if (!pid)
			return mmap_junk(fname, flags, sync_options, file_size);
	}

	for (i = 0; i < mthreads + nthreads; i++) {
		pid = wait(&status);
		if (WIFEXITED(status))
			printf("Child %d exited with status %d\n", pid, WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			printf("Child %d exited with signal %d\n", pid, WTERMSIG(status));
	}

	return 0;
}
