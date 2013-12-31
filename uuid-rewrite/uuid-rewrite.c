#include <uuid/uuid.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include "ctree.h"
#include "crc32c.h"

#define BLOCKSIZE (4096)
static char buf[BLOCKSIZE];
static int csum_size;
static const int syncevery = 4*1024*1024/BLOCKSIZE;

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

static int check_csum_treeblock(void *tb)
{
	char result[csum_size];
	u32 crc = ~(u32)0;

	crc = btrfs_csum_data((char *)tb + BTRFS_CSUM_SIZE,
				crc, BLOCKSIZE - BTRFS_CSUM_SIZE);
	btrfs_csum_final(crc, result);

	return !memcmp(tb, &result, csum_size);
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
	uuid_t uuid_src;
	uuid_t uuid_dest;
	int fd;
	loff_t off;
	loff_t fsize;
	struct stat st;
	int ret;
	time_t ts;
	u64 skipped = 0;
	u64 rewritten = 0;
	int i;
	int is_sb;

	if (argc < 3) {
		printf("Usage: %s srcUUID destUUID image\n", argv[0]);
		exit(1);
	}
	if (uuid_parse(argv[1], uuid_src)) {
		printf("Cannot parse src uuid\n");
		exit(1);
	}
	if (uuid_parse(argv[2], uuid_dest)) {
		printf("Cannot parse dest uuid\n");
		exit(1);
	}
	fd = open(argv[3], O_RDWR | O_EXCL);
	if (fd == -1) {
		perror("open()");
		exit(1);
	}
	/* verify superblock */
	csum_size = btrfs_csum_sizes[BTRFS_CSUM_TYPE_CRC32];
	fstat(fd, &st);
	fsize = st.st_size;
	ts = time(NULL);
	for (off = 0; off < fsize; off += BLOCKSIZE) {
		time_t now = time(NULL);
		struct btrfs_header *hdr;

		if (now - ts > 1) {
			printf("At offset %llu\n",
					(unsigned long long)off);
			ts = now;
		}
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
		is_sb = 0;
		for (i = 0; i < BTRFS_SUPER_MIRROR_MAX; i++) {
			if (btrfs_sb_offset(i) == off) {
				is_sb = 1;
				printf("SB at %llu\n", (unsigned long long)off);
				break;
			}
		}
		/* verify fsid */
		if (memcmp(&hdr->fsid, uuid_src, BTRFS_UUID_SIZE)) {
			/* Not our block */
			skipped++;
			continue;
		}
		/* verify checksum */
		if (is_sb) {
			if (!check_csum_superblock(&hdr->csum)) {
				printf("super block checksum does not match at offset %llu\n",
					(unsigned long long)off);
				exit(1);
			}
		} else {
			if (!check_csum_treeblock(&hdr->csum)) {
				printf("tree block checksum does not match at offset %llu\n",
					(unsigned long long)off);
				exit(1);
			}
		}
		/* verify block offset */
		if (le64_to_cpu(hdr->bytenr) != off) {
			printf("tree block checksum does not match at offset %llu\n",
				(unsigned long long)off);
			exit(1);
		}
		/* rewrite fsid */
		memcpy(&hdr->fsid, uuid_dest, BTRFS_UUID_SIZE);
		if (is_sb) {
			struct btrfs_super_block *sb =
				(struct btrfs_super_block *)buf;
			memcpy(&sb->dev_item.fsid, uuid_dest, BTRFS_UUID_SIZE);
		}
		/* rewrite csum */
		update_block_csum(buf, is_sb);
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
		rewritten++;
		if ((rewritten % syncevery) == 0)
			fdatasync(fd);
next_block:;
	}
	fsync(fd);
	close(fd);
	printf("Skipped blocks   %llu\n", (unsigned long long)skipped);
	printf("Rewritten blocks %llu\n", (unsigned long long)rewritten);
	return 0;
}
