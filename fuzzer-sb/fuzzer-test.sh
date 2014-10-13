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
	sudo mount -t btrfs -o loop,ro img.test $mnt
done
sudo umount $mnt
