#!/bin/sh

function fake() {
	for i in `seq 10`;do
		echo hi $i
	done
	exit
}

function worker() {

echo "this is logtest worker 1" > /dev/tty
date
set -x
./compilebench -i 10 --makej -D /mnt/logtest

}

worker 2>&1 >> logtest1.log
