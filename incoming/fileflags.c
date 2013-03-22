#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#ifndef FS_NOCOMP_FL
#define FS_NOCOMP_FL                    0x00000400 /* Don't compress */
#endif

#ifndef FS_NOCOW_FL
#define FS_NOCOW_FL                     0x00800000 /* Do not cow file */
#endif

struct flags_name {
	unsigned long	flag;
	const char	short_name;
	const char	*long_name;
};

static const struct flags_name flags[]={
	/* new */
	{ FS_NOCOW_FL, 'C', "NOCOW" },
	{ FS_NOCOMP_FL, 'z', "Not_Compressed" },
	/* current */
	{ FS_SECRM_FL, 's', "Secure_Deletion" },
	{ FS_UNRM_FL, 'u' , "Undelete" },
	{ FS_SYNC_FL, 'S', "Synchronous_Updates" },
	{ FS_DIRSYNC_FL, 'D', "Synchronous_Directory_Updates" },
	{ FS_IMMUTABLE_FL, 'i', "Immutable" },
	{ FS_APPEND_FL, 'a', "Append_Only" },
	{ FS_NODUMP_FL, 'd', "No_Dump" },
	{ FS_NOATIME_FL, 'A', "No_Atime" },
	{ FS_COMPR_FL, 'c', "Compression_Requested" },
	{ FS_COMPRBLK_FL, 'B', "Compressed_File" },
	{ FS_DIRTY_FL, 'Z', "Compressed_Dirty_File" },
	{ FS_ECOMPR_FL, 'E', "Compression_Error" },
	{ FS_JOURNAL_DATA_FL, 'j', "Journaled_Data" },
	{ FS_INDEX_FL, 'I', "Indexed_directory" },
	{ FS_NOTAIL_FL, 't', "No_Tailmerging" },
	{ FS_TOPDIR_FL, 'T', "Top_of_Directory_Hierarchies" },
	/* { EXT4_EXTENTS_FL, 'e', "Extents" }, */
	/* { EXT4_HUGE_FILE_FL, 'h', "Huge_file" }, */
};

static unsigned long to_set, to_unset;

unsigned long c2val(char c)
{
	int i;

	for(i=0;i<sizeof(flags)/sizeof(flags[0]);i++) {
		if(flags[i].short_name==c)
			return flags[i].flag;
	}

	printf("Warning: flag '%c' not found\n", c);
	return 0;
}
void list_flags(unsigned long fflags)
{
	int i;

	printf("Flags:\n");
	for(i=0;i<sizeof(flags)/sizeof(flags[0]);i++) {
		if(fflags & flags[i].flag) {
			printf(" %c %s\n",
				flags[i].short_name,
				flags[i].long_name);
		}
	}
}

void set_flag(const char* in)
{
	int i;

	for(i=0;i<strlen(in);i++) {
		to_set |= c2val(in[i]);
		printf(" set %c\n", in[i]);
	}
}

void unset_flag(const char* in)
{
	int i;

	for(i=0;i<strlen(in);i++) {
		to_unset |= c2val(in[i]);
		printf(" unset %c\n", in[i]);
	}
}

int main(int argc, char **argv)
{
        int ret;
	int optind;
	int modify=0;

	if (argc < 2) {
		printf("usage: %s [options] [--] file...\n", argv[0]);
		exit(1);
	}
	to_set = 0;
	to_unset = 0;
	for(optind=1;optind<argc;optind++) {
		char *o=argv[optind];
		if(o[0]=='-' && o[1]=='-') {
			optind++;
			break;
		} else if(o[0]=='-') {
			unset_flag(o+1);
			modify=1;
		} else if(o[0]=='+') {
			set_flag(o+1);
			modify=1;
		} else break;
	}

	for(;optind<argc;optind++) {
		unsigned long fflags;
		int fd = -1;

		if(fd!=-1) close(fd);
		fd = open(argv[optind], O_RDONLY);
		if (fd == -1) {
			perror("open()");
			continue;
		}
		ret = ioctl(fd, FS_IOC_GETFLAGS, &fflags);
		if (ret == -1) {
			perror("ioctl(GETFLAGS)");
			continue;
		}
		if(modify) {
			fflags |= to_set;
			fflags &= ~to_unset;
			ret = ioctl(fd, FS_IOC_SETFLAGS, &fflags);
			if (ret == -1) {
				perror("ioctl(SETFLAGS)");
				continue;
			}
		}
		printf("File: %s\n", argv[optind]);
		list_flags(fflags);
		putchar('\n');
	}

        return 0;
}
