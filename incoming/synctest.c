/*
 * Test and benchmark synchronous operations.
 * Originally written by Andrew Morton
 */

#undef _XOPEN_SOURCE	/* MAP_ANONYMOUS */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/mman.h>

/*
 * Lots of yummy globals!
 */
char *progname, *dirname;
int verbose, use_fsync, use_osync;
int fsync_dir;
int n_threads = 1, n_iters = 100;
int *child_status;
int this_child_index;
int dir_fd;
int show_tids;
int threads_per_dir = 1;
int thread_group;
int do_unlink;
int rename_pass;

#define N_FILES		100
#define UNLINK_LAG	30
#define RENAME_PASSES	3

void show(char *fmt, ...)
{
	if (verbose) {
		va_list ap;

		va_start(ap, fmt);
		vfprintf(stdout, fmt, ap);
		fflush( stdout );
		va_end(ap);
	}
}

/*
 * - Create a file.
 * - Write some data to it
 * - Maybe fsync() it.
 * - Close it
 * - Maybe fsync() its parent dir
 * - rename() it.
 * - maybe fsync() its parent dir
 * - rename() it.
 * - maybe fsync() its parent dir
 * - rename() it.
 * - maybe fsync() its parent dir
 * - UNLINK_LAG files later, maybe unlink it.
 * - maybe fsync() its parent dir
 *
 * Repeat the above N_FILES times
 */

char *mk_dirname(void)
{
	char *ret = malloc(strlen(dirname) + 64);

	sprintf(ret, "%s/%05d", dirname, thread_group);
	return ret;
}

char *mk_filename(int fileno)
{
	char *ret = malloc(strlen(dirname) + 64);

	sprintf(ret, "%s/%05d/%05d-%05d",
			dirname, thread_group, getpid(), fileno);
	return ret;
}

char *mk_new_filename(int fileno, int pass)
{
	char *ret = malloc(strlen(dirname) + 64);

	sprintf(ret, "%s/%05d/%02d-%05d-%05d",
			dirname, thread_group, pass, getpid(), fileno);
	return ret;
}

void sync_dir(void)
{
	if (fsync_dir) {
		show("fsync(%s)\n", dirname);
		if (fsync(dir_fd) < 0) {
			fprintf(stderr, "%s: failed to fsync dir `%s': %s\n",
				progname, dirname, strerror(errno));
			exit(1);
		}
	}
}

void make_dir(void)
{
	char *n = mk_dirname();

	show("mkdir(%s)\n", n);
	if (mkdir(n, 0777) < 0) {
		fprintf(stderr, "%s: Cannot make directory `%s': %s\n",
			progname, n, strerror(errno));
		exit(1);
	}
	free(n);
}

void remove_dir(void)
{
	char *n = mk_dirname();
	show("rmdir(%s)\n", n);
	rmdir(n);
	free(n);
}

void write_stuff_to(int fd, char *name)
{
	static char buf[500000];
	static int to_write = 5000;

	show("write %d bytes to `%s'\n", sizeof(buf), name);
	if (write(fd, buf, to_write) != to_write) {
		fprintf(stderr, "%s: failed to write %d bytes to `%s': %s\n",
			progname, to_write, name, strerror(errno));
		exit(1);
	}

	to_write *= 1.1;
	if (to_write > 250000)
		to_write = 5000;
}

void unlink_one_file(int fileno, int pass)
{
	if (do_unlink) {
		char *name = mk_new_filename(fileno, pass);

		show("unlink(%s)\n", name);
		if (unlink(name) < 0) {
			fprintf(stderr, "%s: failed to unlink `%s': %s\n",
				progname, name, strerror(errno));
			exit(1);
		}
		sync_dir();
		free(name);
	}
}

void do_one_file(int fileno)
{
	char *name = mk_filename(fileno);
	int fd, flags;

	flags = O_RDWR|O_CREAT|O_TRUNC;
	if (use_osync)
		flags |= O_SYNC;

	show("open(%s)\n", name);
	fd = open(name, flags, 0666);
	if (fd < 0) {
		fprintf(stderr, "%s: failed to create file `%s': %s\n",
			progname, name, strerror(errno));
		exit(1);
	}

	write_stuff_to(fd, name);

	if (use_fsync) {
		show("fsync(%s)\n", name);
		if (fsync(fd) < 0) {
			fprintf(stderr, "%s: failed to fsync `%s': %s\n",
				progname, name, strerror(errno));
			exit(1);
		}
	}

	show("close(%s)\n", name);
	if (close(fd) < 0) {
		fprintf(stderr, "%s: failed to close `%s': %s\n",
			progname, name, strerror(errno));
		exit(1);
	}

	sync_dir();

	for (rename_pass = 0; rename_pass < RENAME_PASSES; rename_pass++) {
		char *newname = mk_new_filename(fileno, rename_pass);

		show("rename(%s, %s)\n", name, newname);
		if (rename(name, newname) < 0) {
			fprintf(stderr,
				"%s: failed to rename `%s' to `%s': %s\n",
				progname, name, newname, strerror(errno));
			exit(1);
		}
		sync_dir();
		free(name);
		name = newname;
	}
	rename_pass--;
	free(name);
}

void do_child(void)
{
	int fileno;
	char *dn = mk_dirname();
	int dotcount;

	dir_fd = open(dn, O_RDONLY);
	if (dir_fd < 0) {
		fprintf(stderr, "%s: failed to open dir `%s': %s\n",
			progname, dn, strerror(errno));
		exit(1);
	}
	free(dn);

	dotcount = N_FILES / 10;
	if (dotcount == 0)
		dotcount = 1;

	for (fileno = 0; fileno < N_FILES; fileno++) {
		if (fileno % dotcount == 0) {
			printf(".");
			fflush(stdout);
		}
		do_one_file(fileno);
		if (fileno >= UNLINK_LAG)
			unlink_one_file(fileno - UNLINK_LAG, RENAME_PASSES - 1);
	}
	for (fileno = N_FILES - UNLINK_LAG; fileno < N_FILES; fileno++)
		unlink_one_file(fileno, RENAME_PASSES - 1);
}

void doit(void)
{
	int child;
	int children_left;

	child_status = (int *)mmap(	0,
				n_threads * sizeof(*child_status),
				PROT_READ|PROT_WRITE,
				MAP_SHARED|MAP_ANONYMOUS,
				-1,
				0);
	if (child_status == MAP_FAILED) {
		perror("mmap");
		exit(1);
	}

	memset(child_status, 0, n_threads * sizeof(*child_status));

	thread_group = -1;
	for (this_child_index = 0;
			this_child_index < n_threads; this_child_index++)
	{
		if (this_child_index % threads_per_dir == 0) {
			thread_group++;
			make_dir();
		}

		if (fork() == 0) {
			int iter;

			for (iter = 0; iter < n_iters; iter++)
				do_child();
			child_status[this_child_index] = 1;
			exit(0);
		}
	}

	/* Parent */
	children_left = n_threads;
	while (children_left) {
		int status;

		if( wait3(&status, 0, 0) < 0 ) {
			if( errno != EINTR ) {
				perror("wait3");
				exit(1);
			}
			continue;
		}
		for (child = 0; child < n_threads; child++) {
			if (child_status[child] == 1) {
				child_status[child] = 2;
				printf("*");
				fflush(stdout);
				children_left--;
			}
		}
	}
	for (thread_group = 0; 
			thread_group < ( n_threads / threads_per_dir ); 
			thread_group++ )
		remove_dir();

	printf("\n");
}

void usage(void)
{
	fprintf(stderr,
		"Usage: %s [-fFosuv] [-p threads-pre-dir ][-n iters] [-t threads] dirname\n",
			progname);
	fprintf(stderr, "        -f:    Use fsync() on close\n"); 
	fprintf(stderr, "        -F:    Use fsync() on parent dir\n"); 
	fprintf(stderr, "        -n:    Number of iterations\n");
	fprintf(stderr, "        -o:    Open files O_SYNC\n");
	fprintf(stderr, "        -p:    Number of threads per directory\n");
	fprintf(stderr, "        -t:    Number of threads\n");
	fprintf(stderr, "        -u:    Unlink files during test\n");
	fprintf(stderr, "        -v:    Verbose\n"); 
	fprintf(stderr, "   dirname:    Directory to run tests in\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	int c;

	progname = argv[0];
	while ((c = getopt(argc, argv, "vFfout:n:p:")) != -1) {
		switch (c) {
		case 'f':
			use_fsync++;
			break;
		case 'F':
			fsync_dir++;
			break;
		case 'n':
			n_iters = strtol(optarg, NULL, 10);
			break;
		case 'o':
			use_osync++;
			break;
		case 'p':
			threads_per_dir = strtol(optarg, NULL, 10);
			break;
		case 't':
			n_threads = strtol(optarg, NULL, 10);
			break;
		case 'u':
			do_unlink++;
			break;
		case 'v':
			verbose++;
			break;
		}
	}

	if (optind == argc)
		usage();
	dirname = argv[optind++];
	if (optind != argc)
		usage();

	doit();
	exit(0);
}
