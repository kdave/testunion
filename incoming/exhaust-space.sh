#!/bin/sh -x

src=/dev/zero

cnt=1

function round () {

	dd if=$src of=zero-$$-$cnt bs=10M
	cnt=$(($cnt+1))
	dd if=$src of=zero-$$-$cnt bs=1M
	cnt=$(($cnt+1))
	dd if=$src of=zero-$$-$cnt bs=1K
	cnt=$(($cnt+1))
	dd if=$src of=zero-$$-$cnt bs=1c
	sync
	sync
	sync
}

round
round
round

for i in `seq 10`; do
	touch empty-$$-$i
	sync
done
