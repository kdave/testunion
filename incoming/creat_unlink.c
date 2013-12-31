/* gcc creat_unlink.c -o creat_unlink */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define UNUSED          __attribute__ ((unused))
#define PATH_SIZE       100

char *tst_dir="tst_dir";

int initialize(void)
{
	int ret;

	ret = mkdir(tst_dir, 0775);
	if (ret) {
		perror("init fail - mkdir failed\n");
		goto err;
	}

	ret = chdir(tst_dir);
	if (ret) {
		perror("init fail - chdir failed\n");
		rmdir(tst_dir);
	}
err:
	return ret;
}

int cleanup(void)
{
	int ret;

	ret = chdir("..");
	if (ret) {
		perror("cleanup fail - chdir failed\n");
		goto err;
	}

	ret = rmdir(tst_dir);
	if (ret)
		perror("cleanup fail - rmdir failed\n");

err:
	return ret;
}

int get_start_time(struct timeval *tv)
{
	if (gettimeofday(tv, NULL)) {
		perror("get start time failed.\n");
		return 1;
	}

	return 0;
}

void account_time(struct timeval *stv, struct timeval *etv, int files)
{
	if (gettimeofday(etv, NULL)) {
		perror("get end time failed.\n");
	} else if (files) {
		double ts;

		while (etv->tv_usec < stv->tv_usec) {
			etv->tv_sec--;
			etv->tv_usec += 1000000;
		}

		etv->tv_usec -= stv->tv_usec;
		etv->tv_sec -= stv->tv_sec;

		while (etv->tv_usec > 1000000) {
			etv->tv_usec -= 1000000;
			etv->tv_sec++;
		}

		ts = (double)etv->tv_usec / (double)1000000 + etv->tv_sec;
		printf("\tTotal files: %d\n", files);
		printf("\tTotal time: %lf\n", ts);

		ts /= files;
		printf("\tAverage time: %lf\n", ts);
	} else
		perror("Didn't create/delete any files");

}

int create_files(int nfiles)
{
	int i, fd;
	char fpath[PATH_SIZE];
	struct timeval stv, etv;

	printf("Create files:\n");
	if (get_start_time(&stv))
		return 0;

	for (i = 0; i < nfiles; i++) {
		sprintf(fpath, "%d", i);
		fd = creat(fpath, 0555);
		if (fd < 0) {
			perror("creat file failed.\n");
			break;
		}
		close(fd);
	}

	account_time(&stv, &etv, i);

	return i;
}

int unlink_files(int nfiles)
{
	int i, ret = 0;
	char fpath[PATH_SIZE];
	struct timeval stv, etv;

	printf("Delete files:\n");
	if (get_start_time(&stv))
		return 1;

	for (i = 0; i < nfiles; i++) {
		sprintf(fpath, "%d", i);
		ret = unlink(fpath);
		if (ret)
			break;
	}

	account_time(&stv, &etv, i);

	return ret;
}

int main(int argc, char **argv)
{
	int nfiles, n_done, ret;

	/* parse the options */
	if (argc <= 2) {
		fprintf(stderr, "options is wrong.\n");
		fprintf(stderr, "%s [nfiles] [tst_dir]\n", argv[0]);
		exit(1);
	}

	nfiles = atoi(argv[1]);
	if (nfiles <= 0) {
		fprintf(stderr, "option nfiles is wrong.");
		exit(1);
	}

	if (argc >= 3) {
		tst_dir = strdup(argv[2]);
	}

	/* initialize - create the test directory and change the current dir */
	ret = initialize();
	if (ret)
		exit(1);

	n_done = create_files(nfiles);
	if (n_done != nfiles) {
		unlink_files(n_done);
		cleanup();
		exit(1);
	}

	ret = unlink_files(n_done);
	if (ret)
		exit(1);

	ret = cleanup();
	if (ret)
		exit(1);

	return 0;
}
