#!/bin/sh -x
# test many devices manipulation

count=4096
count=8
fsize=100M
fsdev=/dev/loop1
fsmnt=mnt-loop1
alldevs=''
fillsrc=linux-2.6

function setup() {
	modprobe loop max_loop=$((count+1)) max_part=63
	mkdir $fsmnt
}
function setdown() {
	rmmod loop
}
function loopcreate() {
	dd if=/dev/zero bs=1 count=1 seek=$fsize of=loop-$1
}
function loopsetup() {
	losetup /dev/loop$i loop-$i
}

function setupall() {
	for i in `seq $count`; do
		if ! [ -f loop-$i ]; then
			loopcreate $i
		fi
		loopsetup
		alldevs="$alldevs /dev/loop$i"
	done
}
function setdownall() {
	for i in `seq $count`; do
		losetup -d /dev/loop$i
	done
}
function adddev() {
	btrfs dev add /dev/loop$i $fsmnt
}
function rmdev() {
	btrfs dev delete /dev/loop$i $fsmnt
}
function fillwithdata() {
	echo "fill with data from $fillsrc"
	cp -a $fillsrc $fsmnt
}
function balance() {
	btrfs fi show $fsmnt
	btrfs fi balance $fsmnt
}

# main
setup
setupall
losetup -a
if ! losetup $fsdev | grep -q `pwd`;then
	echo refuse to mkfs on foreign loop, check your setup
	exit 1
else
	mkfs.btrfs -m raid10 -d raid10 $alldevs
fi
# start!
fillwithdata
rmdev 3
rmdev 5
rmdev 4
balance
addev 3
addev 4
addev 5
balance
setdownall
setdown
