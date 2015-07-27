#include <sys/fcntl.h>
#include <sys/stat.h>
#include "ctree.h"
#include "crc32c.h"

#define BLOCKSIZE (4096)
static char buf[BLOCKSIZE];
static int csum_size;

u32 btrfs_csum_data(char *data, u32 seed, size_t len)
{
	return crc32c(seed, data, len);
}

void btrfs_csum_final(u32 crc, char *result)
{
	put_unaligned_le32(~crc, result);
}

static int check_csum_superblock(void *sb)
{
	char result[csum_size];
	u32 crc = ~(u32)0;

	crc = btrfs_csum_data((char *)sb + BTRFS_CSUM_SIZE,
				crc, BTRFS_SUPER_INFO_SIZE - BTRFS_CSUM_SIZE);
	btrfs_csum_final(crc, result);

	return !memcmp(sb, &result, csum_size);
}

static void update_block_csum(void *block, int is_sb)
{
	char result[csum_size];
	struct btrfs_header *hdr;
	u32 crc = ~(u32)0;

	if (is_sb) {
		crc = btrfs_csum_data((char *)block + BTRFS_CSUM_SIZE, crc,
				BTRFS_SUPER_INFO_SIZE - BTRFS_CSUM_SIZE);
	} else {
		crc = btrfs_csum_data((char *)block + BTRFS_CSUM_SIZE, crc,
				BLOCKSIZE - BTRFS_CSUM_SIZE);
	}
	btrfs_csum_final(crc, result);
	memset(block, 0, BTRFS_CSUM_SIZE);
	hdr = (struct btrfs_header *)block;
	memcpy(&hdr->csum, result, csum_size);
}

int main(int argc, char **argv) {
	int fd;
	loff_t fsize;
	loff_t off;
	struct stat st;
	int ret;
	struct btrfs_header *hdr;
	int write_changes = 0;
	int optind = 1;

	if (argc < 2) {
		printf("Usage: %s [--write] blockoffset /dev/sdx\n", argv[0]);
		exit(1);
	}
	if (strcmp(argv[optind], "--write") == 0) {
		printf("Will write changes\n");
		write_changes = 1;
		optind++;
	}
	off = atoi(argv[optind++]);
	if (optind >= argc) {
		printf("Usage: %s [--write] blockoffset /dev/sdx\n", argv[0]);
		exit(1);
	}
	fd = open(argv[optind++], O_RDWR | O_EXCL);
	if (fd == -1) {
		perror("open()");
		exit(1);
	}

	/* verify superblock */
	csum_size = btrfs_csum_sizes[BTRFS_CSUM_TYPE_CRC32];
	fstat(fd, &st);
	fsize = st.st_size;

	ret = pread64(fd, buf, BLOCKSIZE, off);
	if (ret <= 0) {
		printf("pread error %d at offset %llu\n",
				ret, (unsigned long long)off);
		exit(1);
	}
	if (ret != BLOCKSIZE) {
		printf("pread error at offset %llu: read only %d bytes\n",
				(unsigned long long)off, ret);
		exit(1);
	}
	hdr = (struct btrfs_header *)buf;
	/* verify checksum */
	if (!check_csum_superblock(&hdr->csum)) {
		printf("super block checksum does not match at offset %llu\n",
				(unsigned long long)off);
	} else {
		printf("super block checksum is ok\n");
	}
	if (write_changes) {
		printf("updating csum\n");
		/* rewrite csum */
		update_block_csum(buf, off == 16 * 4096);
		ret = pwrite64(fd, buf, BLOCKSIZE, off);
		if (ret <= 0) {
			printf("pwrite error %d at offset %llu\n",
					ret, (unsigned long long)off);
			exit(1);
		}
		if (ret != BLOCKSIZE) {
			printf("pwrite error at offset %llu: written only %d bytes\n",
					(unsigned long long)off, ret);
			exit(1);
		}
		fsync(fd);
	if (!check_csum_superblock(&hdr->csum)) {
		printf("2 super block checksum does not match at offset %llu\n",
				(unsigned long long)off);
	} else {
		printf("2 super block checksum is ok\n");
	}
	}
	close(fd);

	return 0;
}
