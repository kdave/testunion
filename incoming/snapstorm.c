/* gcc -o snapstorm snapstorm.c -pthread */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <dirent.h>
#include <inttypes.h>
#include <string.h>

typedef int64_t __s64;
typedef uint64_t __u64;

#ifndef BTRFS_PATH_NAME_MAX
#define BTRFS_PATH_NAME_MAX 4087
#endif

#ifndef BTRFS_SUBVOL_NAME_MAX
#define BTRFS_SUBVOL_NAME_MAX 4039
#endif

#ifndef BTRFS_IOCTL_MAGIC
#define BTRFS_IOCTL_MAGIC 0x94
#endif

#define BTRFS_IOC_SNAP_CREATE_V2 _IOW(BTRFS_IOCTL_MAGIC, 23, \
		struct __local__btrfs_ioctl_vol_args_v2)

struct __local__btrfs_ioctl_vol_args_v2 {
	__s64 fd;
	__u64 transid;
	__u64 flags;
	__u64 unused[4];
	char name[BTRFS_SUBVOL_NAME_MAX + 1];
};

pthread_mutex_t m_start;
pthread_cond_t c_start;
pthread_mutex_t m_end;
static int can_start = 0;
static int nr_ready = 0;
static int nr_finished = 0;

/* arg holders */
static int opt_readonly;
static char opt_source[4096];
static int opt_rounds = 1;

static int test_issubvolume(char *path) {
	struct stat	st;
	int		res;

	res = stat(path, &st);
	if(res < 0 )
		return -1;

	return (st.st_ino == 256) && S_ISDIR(st.st_mode);
}

int open_file_or_dir(const char *fname) {
	int ret;
	struct stat st;
	DIR *dirstream;
	int fd;

	ret = stat(fname, &st);
	if (ret < 0) {
		return -1;
	}
	if (S_ISDIR(st.st_mode)) {
		dirstream = opendir(fname);
		if (!dirstream) {
			return -2;
		}
		fd = dirfd(dirstream);
	} else {
		fd = open(fname, O_RDWR);
	}
	if (fd < 0) {
		return -3;
	}
	return fd;
}

void do_snap(const char *target) {
	int fd, fddst, res;
	struct __local__btrfs_ioctl_vol_args_v2 args;

	fddst = open_file_or_dir(".");
	if(fddst<0) {
		printf("do_snap: no dest\n");
		return;
	}
	if(!test_issubvolume(opt_source)) {
		printf("do_snap: source not subvolume\n");
		return;
	}
	fd = open_file_or_dir(opt_source);
	if(fd<0) {
		printf("do_snap: no src\n");
		return;
	}
	args.fd=fd;
	strncpy(args.name, target, BTRFS_SUBVOL_NAME_MAX);
	res = ioctl(fddst, BTRFS_IOC_SNAP_CREATE_V2, &args);
	close(fd);
	close(fddst);
	if(res<0)
		printf("do_snap: ioctl %d\n");
}

void *t_worker(void *arg) {
	long i = (long)(arg);
	char target[4096];

	printf("[%ld] wait...\n", i);
	pthread_mutex_lock(&m_start);
	nr_ready++;
	printf("[%ld] nr_ready %ld\n", nr_ready);
	while(!can_start)
		pthread_cond_wait(&c_start, &m_start);
	pthread_mutex_unlock(&m_start);

	snprintf(target, sizeof(target), "%s-%ld-%lu", i, time(NULL));
	printf("[%ld] snap %s\n", i, target);
	do_snap(target);
	printf("[%ld] done...\n", i);
	pthread_mutex_lock(&m_end);
	nr_finished++;
	pthread_mutex_unlock(&m_end);
	return 0;
}

int main(int argc, char **argv) {
	int nr_threads = 10;
	long i;

	while(1) {
		int c=getopt(argc, argv, "rc:n:");
		if(c<0) break;
		switch(c) {
		case 'r': opt_readonly=1; break;
		case 'c':
			  nr_threads=atoi(optarg);
			  if(nr_threads <= 0 || nr_threads > 4096)
				  nr_threads = 10;
			  break;
		case 'n':
			  opt_rounds=atoi(optarg);
			  if(opt_rounds <= 0 || opt_rounds > 4096)
				  opt_rounds = 1;
			  break;
		}
	}
	argc -= optind;

	if(argc != 2) {
		printf("Usage: snapstorm [] from to\n");
		printf("\t\t-r\treadonly snapshot\n");
		exit(1);
	}
	strcpy(opt_source, argv[1]);

	pthread_cond_init(&c_start, NULL);
	pthread_mutex_init(&m_start, NULL);

	printf("MAIN: starting %ld threads\n", nr_threads);
	for(i=0;i<nr_threads;i++) {
		pthread_t t;

		pthread_create(&t, NULL, t_worker, (void*)i);
		pthread_detach(t);
	}
	pthread_mutex_lock(&m_start);
	while (nr_ready < nr_threads) {
		printf("MAIN: waiting for last %d to start\n",
				nr_threads - nr_ready);
		pthread_mutex_unlock(&m_start);
		sleep(1);
		pthread_mutex_lock(&m_start);
	}
	pthread_mutex_unlock(&m_start);

	printf("MAIN: all set ... go!\n");
	pthread_mutex_lock(&m_start);
	can_start = 1;
	pthread_cond_broadcast(&c_start);
	pthread_mutex_unlock(&m_start);

	pthread_mutex_lock(&m_end);
	while(nr_finished < nr_threads) {
		pthread_mutex_unlock(&m_end);
		sleep(1);
		pthread_mutex_lock(&m_end);
	}
	printf("MAIN: done\n");

	return 0;
}
