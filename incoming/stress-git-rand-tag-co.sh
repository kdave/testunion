#!/bin/sh

for i in $(git tag|shuf); do
	echo TAG: $i
	time git checkout "$i"
done
