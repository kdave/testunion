#!/bin/sh

function fake() {
	for i in `seq 10`;do
		echo hi3 $i
	done
	exit
}

function worker() {

echo "this is logtest worker 3" > /dev/tty
date
set -x

while ! [ -d /mnt/logtest/kernel-0 ];do
	sleep 10
done

for x in `seq 1 2000` ; do
	btrfs subvol snap /mnt/logtest /mnt/logtest/snap$x
	sleep 0.5
done

}

worker 2>&1 >> logtest3.log
