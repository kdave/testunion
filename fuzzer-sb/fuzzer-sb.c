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

static void fuzz_sb_step(void *block, int step)
{
	struct btrfs_super_block *sb = (struct btrfs_super_block *)block;
	static const struct fuzz_ranges {
		/* Inclusive start */
		int start;
		/* Exclusive end */
		int end;
		/* Number of bits to fuzz */
		int width;
	} ranges[] = {
		{ .start = offsetof(struct btrfs_super_block, bytenr),
		  .end = offsetof(struct btrfs_super_block, dev_item),
		  .width = 4
		},
		/* skip dev_item */
		/* skip label */
		{ .start = offsetof(struct btrfs_super_block, cache_generation),
		  .end = offsetof(struct btrfs_super_block, reserved),
		  .width = 4
		},
		/* skip reserved[31] */
		{ .start = offsetof(struct btrfs_super_block, sys_chunk_array),
		  .end = sizeof(struct btrfs_super_block),
		  .width = 4
		}
	};
	int i;
	int now = 0;

	for (i = 0; i < ARRAY_SIZE(ranges); i++) {
		int s = ranges[i].start;
		int e = ranges[i].end;
		int w = ranges[i].width;
		int j;

		for (j = 0; j < w; j++) {
			if (now >= step)
				break;
		}
		if (now < step)
			continue;

		printf("Fuzz in range [%d-%d) w=%d step=%d\n", s, e, w, now);
	}
}

int main(int argc, char **argv) {
	int fd;
	loff_t off;
	loff_t fsize;
	struct stat st;
	int ret;
	int is_sb;
	struct btrfs_header *hdr;
	int step;

	if (argc < 2) {
		printf("Usage: %s image step\n", argv[0]);
		exit(1);
	}
	fd = open(argv[1], O_RDWR | O_EXCL);
	if (fd == -1) {
		perror("open()");
		exit(1);
	}
	step = atoi(argv[2]);
	printf("Starting fuzz with step %d\n", step);

	/* verify superblock */
	csum_size = btrfs_csum_sizes[BTRFS_CSUM_TYPE_CRC32];
	fstat(fd, &st);
	fsize = st.st_size;
	off = 16*4096;

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
		printf("super block checksum does not match at offset %llu, will be corrected\n",
				(unsigned long long)off);
	} else {
		printf("super block checksum is ok\n");
	}
	printf("Fuzzing\n");
	fuzz_sb_step(buf, step);
	printf("updating csum\n");
	/* rewrite csum */
	update_block_csum(buf, 1);
	/* write block */
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
	close(fd);
	return 0;
}
