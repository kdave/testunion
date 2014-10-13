#!/bin/sh

loops=10
start=${1:-1}
mnt=mnt
mode=sparse

fs=$(stat -f --format=%T .)
[ "$fs" = "btrfs" ] && mode=reflink

mkdir -p $mnt

for i in `seq $start $loops`; do
	echo Loop $i
	cp --${mode}=always img.orig img.test
	echo Fuzz
	./fuzzer-sb img.test $i
	sync
	echo Test mount
	sudo umount $mnt
	lo=$(sudo losetup -f --show img.test)
	sudo mount -t btrfs -o rw $lo $mnt
	sudo losetup -d $lo
done
sudo umount $mnt
