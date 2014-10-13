#!/bin/sh

loops=${1:-10}
start=${2:-0}
mnt=mnt
mode=sparse

fs=$(stat -f --format=%T .)
[ "$fs" = "btrfs" ] && mode=reflink

mkdir -p $mnt

for i in `seq $start $loops`; do
	echo Loop $i
	sudo umount $mnt
	cp --${mode}=always img.orig img.test
	if [ "$i" != 0 ]; then
		echo Fuzz
		./fuzzer-sb img.test $i
	else
		echo Skip fuzzing for iteration 0
	fi
	echo Test mount
	lo=$(sudo losetup -f --show img.test)
	sync
	sudo mount -t btrfs -o rw $lo $mnt
	ret=$?
	sudo umount $mnt
	echo "Fsck"
	sudo btrfsck $lo
	sudo losetup -d $lo
	if [ "$i" = 0 ]; then
		if [ "$ret" = 0 ]; then
			echo "Base image is ok"
		else
			echo "Cannot mount the base image, exit"
			break
		fi
	fi
done
