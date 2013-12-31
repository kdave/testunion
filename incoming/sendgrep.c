#include <inttypes.h>
typedef uint8_t u8;
typedef uint64_t u64;
#include <btrfs/send-stream.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

char *pathstring;
size_t pathlen;


int my_subvol(const char *path, const u8 *uuid, u64 ctransid, void *user) {	
	return 0;
}

int my_snapshot(const char *path, const u8 *uuid, u64 ctransid, const u8 *parent_uuid, u64 parent_ctransid, void *user) {
	return 0;
}

int my_mkfile(const char *path, void *user) {
	if (!memcmp(pathstring, path, pathlen))
		printf("mkfile %s\n", path);
	return 0;
}

int my_mkdir(const char *path, void *user) {
	if (!memcmp(pathstring, path, pathlen))
		printf("mkdir %s\n", path);
	return 0;
}

int my_mknod(const char *path, u64 mode, u64 dev, void *user) {
	if (!memcmp(pathstring, path, pathlen))
		printf("mknod: %s mode: %llx dev: %llx\n", path, (unsigned long long)mode, (unsigned long long)dev);
	return 0;
}

int my_mkfifo(const char *path, void *user) {
	if (!memcmp(pathstring, path, pathlen))
		printf("mkfifo %s\n", path);
	return 0;

}

int my_mksock(const char *path, void *user) {
	if (!memcmp(pathstring, path, pathlen))
		printf("mksock %s\n", path);
	return 0;
}

int my_symlink(const char *path, const char *lnk, void *user) {
	if (!memcmp(pathstring, path, pathlen) || !memcmp(pathstring, lnk, pathlen))
		printf("mksymlink %s to %s\n", path, lnk);
	return 0;
}

int my_rename(const char *from, const char *to, void *user) {
	if (!memcmp(pathstring, from, pathlen) || !memcmp(pathstring, to, pathlen))
		printf("rename %s to %s\n", from, to);
	return 0;
}
int my_link(const char *path, const char *lnk, void *user) {
	if (!memcmp(pathstring, path, pathlen) || !memcmp(pathstring, lnk, pathlen))
		printf("mkdir %s to %s\n", path, lnk);
	return 0;
}
int my_unlink(const char *path, void *user){
	if (!memcmp(pathstring, path, pathlen))
		printf("unlink %s\n", path);
	return 0;
}
int my_rmdir(const char *path, void *user){
	if (!memcmp(pathstring, path, pathlen))
		printf("rmdir %s\n", path);
	return 0;
}
int my_write(const char *path, const void *data, u64 offset, u64 len, void *user){
	if (!memcmp(pathstring, path, pathlen))
		printf("write %s, len %zx, offset %zx\n", path, len, offset);
	return 0;
}
int my_clone(const char *path, u64 offset, u64 len, const u8 *clone_uuid, u64 clone_ctransid, const char *clone_path, u64 clone_offset, void *user){
	if (!memcmp(pathstring, path, pathlen) || !memcmp(pathstring, clone_path, pathlen))
		printf("clone %s to %s, len %zx, srcoffset %zx, destoffset %zx\n", path, clone_path, len, offset, clone_offset);
	return 0;
}
int my_set_xattr(const char *path, const char *name, const void *data, int len, void *user){
	return 0;
}
int my_remove_xattr(const char *path, const char *name, void *user){
	return 0;
}
int my_truncate(const char *path, u64 size, void *user){
	return 0;
}
int my_chmod(const char *path, u64 mode, void *user){
	return 0;
}
int my_chown(const char *path, u64 uid, u64 gid, void *user){
	return 0;
}
int my_utimes(const char *path, struct timespec *at, struct timespec *mt, struct timespec *ct, void *user){
	return 0;
}
int my_update_extent(const char *path, u64 offset, u64 len, void *user){
	return 0;
}

struct btrfs_send_ops myops = {
	.subvol = my_subvol,
	.snapshot = my_snapshot,
	.mkfile = my_mkfile,
	.mkdir = my_mkdir,
	.mknod = my_mknod,
	.mkfifo = my_mkfifo,
	.mksock = my_mksock,
	.symlink = my_symlink,
	.rename = my_rename,
	.link = my_link,
	.unlink = my_unlink,
	.rmdir = my_rmdir,
	.write = my_write,
	.clone = my_clone,
	.set_xattr = my_set_xattr,
	.remove_xattr = my_remove_xattr,
	.truncate = my_truncate,
	.chmod = my_chmod,
	.chown = my_chown,
	.utimes = &my_utimes,
	.update_extent = my_update_extent,
};

int main(int argc, char *argv[]) {
	if (3 != argc) {
		fprintf(stderr, "Usage: %s path sendfile\n", argv[0]);
		return 1;
	}
	pathstring=argv[1];
	pathlen=strlen(argv[1]);

	int fd=open(argv[2], O_RDONLY);
	if (0 > fd) {
		perror("Unable to open stream file");
		return 1;
	}
	
	btrfs_read_and_process_send_stream(fd, &myops, NULL, 0);

	close(fd);
}

