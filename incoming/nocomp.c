#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>

#include <linux/types.h>
#include <stdio.h>
#include <errno.h>

#ifndef FS_IOC_SETFLAGS
#define FS_IOC_SETFLAGS                 _IOW('f', 2, long)
#warning defining SETFLAGS locally
#endif

#ifndef FS_IOC_GETFLAGS
#define FS_IOC_GETFLAGS                 _IOR('f', 1, long)
#warning defining GETFLAGS locally
#endif

#ifndef FS_NOCOW_FL
#define FS_NOCOW_FL                     0x00800000 /* Do not cow file */
#endif

#ifndef FS_COMPR_FL
#define FS_COMPR_FL                     0x00000004 /* Compress file */
#endif

#ifndef FS_NOCOMP_FL
#define FS_NOCOMP_FL                    0x00000400 /* Don't compress */
#endif


int main(int argc, char **argv)
{
        int fd;
        int r;
	long flags;

	if (argc < 2) {
		printf("usage: %s file\n", argv[0]);
		exit(1);
	}

        fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open()");
		return 1;
	}

	r = ioctl(fd, FS_IOC_GETFLAGS, &flags);
	if (r == -1) {
		perror("ioctl(GETFLAGS)");
		return 1;
	} else {
		printf("file flags: 0x%lx\n", flags);
		if(flags & FS_NOCOW_FL) printf(" NOCOW is on\n");
		else printf(" NOCOW is on\n");
		if(flags & FS_NOCOMP_FL) printf(" NOCOMP is on\n");
		else printf(" NOCOMP is on\n");
		if(flags & FS_COMPR_FL) printf(" COMPR is on\n");
		else printf(" COMPR is on\n");
	}

	printf("Set NOCOMP flag for %s\n", argv[1]);
	flags |= FS_NOCOMP_FL;
        r = ioctl(fd, FS_IOC_SETFLAGS, &flags);
	printf("ioctl returned: %d\n", r);
	if (r == -1) {
		perror("ioctl()");
		return 1;
	}

        return 0;
}
