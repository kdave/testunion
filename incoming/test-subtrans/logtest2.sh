#!/bin/sh

function fake() {
	for i in `seq 10`;do
		echo hi2 $i
	done
	exit
}

function worker() {

echo "this is logtest worker 2" > /dev/tty
date
set -x

while ! [ -d /mnt/logtest/kernel-0 ];do
	sleep 10
done

./synctest -f -u -n 200 -t 3 /mnt/logtest

}

worker 2>&1 >> logtest2.log
