#!/bin/sh

pwd=`pwd`

for i in $(git tag|shuf); do
	echo TAG: $i
	time git checkout -f "$i"
	btrfs subvol snap . `pwd`-$i
done
