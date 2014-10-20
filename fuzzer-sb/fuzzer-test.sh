#!/bin/sh

loops=${1:-10}
start=${2:-0}
mnt=mnt
mode=sparse
verbosity=1

fs=$(stat -f --format=%T .)
[ "$fs" = "btrfs" ] && mode=reflink

mkdir -p $mnt logs

function msg() {
	if [ "$1" -le "$verbosity" ]; then
		shift
		echo "$@"
		return 0
	fi
	return 1
}

function msgpipe() {
	if [ "$1" -le "$verbosity" ]; then
		cat
	fi
}

sudo umount $mnt
for i in `seq $start $loops`; do
	msg 1 "Loop $i"
	cp --${mode}=always img.orig img.test
	if [ "$i" != 0 ]; then
		msg 2 "Fuzz"
		./fuzzer-sb img.test $i 2>&1 | msgpipe 3
		btrfs-show-super img.test > logs/sb-dump-$(printf %04d $i).txt
	else
		msg 1 "Skip fuzzing for iteration 0"
	fi
	msg 2 "Test mount"
	lo=$(sudo losetup -f --show img.test)
	sync
	msg 1 "MD5 sum of test image" && md5sum img.test
	sudo mount -t btrfs -o rw $lo $mnt
	cat /proc/mounts|grep loop
	ret=$?
	if [ "$ret" = 0 ]; then
		# must pass
		sudo umount $mnt
		msg 1 "Mount ok"
		msg 2 "Fsck"
		sudo btrfsck $lo 2>&1 | msgpipe 3
	else
		exit
	fi
	sudo losetup -d $lo
	if [ "$i" = 0 ]; then
		if [ "$ret" = 0 ]; then
			msg 1 "Base image is ok"
		else
			msg 1 "Cannot mount the base image, exit"
			break
		fi
	fi
done
