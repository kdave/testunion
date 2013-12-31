all: uuid-rewrite

uuid-rewrite: uuid-rewrite.c ctree.h crc32c.c crc32c.h kerncompat.h
	gcc -o uuid-rewrite uuid-rewrite.c crc32c.c -luuid

fix-sb-csum: fix-sb-csum.c ctree.h crc32c.c crc32c.h kerncompat.h
	gcc -o fix-sb-csum fix-sb-csum.c crc32c.c

test: uuid-rewrite
	cp --sparse=always img.orig img.test
	-./btrfs-show-super img.test | grep fsid
	./uuid-rewrite 72771307-15b4-4389-8bfa-c65880986bb4 11111111-2222-3333-4444-555555555555 img.test
	sync
	-./btrfs-show-super img.test | grep fsid
	sync
	-hexdump img.test > img.test.hexdump
	-sudo umount mnt
	-sudo mount -t btrfs -o loop,ro img.test mnt
	-sudo umount mnt

test-csum: fix-sb-csum
	cp --sparse=always img.orig img.test
	./fix-sb-csum img.test
	./corrupt-sb-csum img.test
	./fix-sb-csum img.test
	./fix-sb-csum img.test
