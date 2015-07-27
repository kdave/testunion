all: fuzzer-sb

fuzzer-sb: fuzzer-sb.c ctree.h crc32c.c crc32c.h kerncompat.h
	gcc -o fuzzer-sb fuzzer-sb.c crc32c.c -luuid

fixup-block-csum: fixup-block-csum.c crc32c.c crc32c.h kerncompat.h
	gcc -o $@ fixup-block-csum.c crc32c.c -luuid

test: fuzzer-sb
	-./fuzzer-test.sh
