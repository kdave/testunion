#!/bin/bash -
# -*- Shell-script -*-
#
# simulate filesystem aging in subvolumes by git activity

nconcurrent=50
content=~/DATA_SOURCES/linux-2.6
stagger=yes
fillzero=false
progname=`basename "$0"`
btrfs=/usr/local/bin/btrfs

while getopts "c:n:sz" c; do
    case $c in
    c)
	content=$OPTARG
	;;
    n)
	nconcurrent=$OPTARG
	;;
    s)
	stagger=no
	;;
    z)
        fillzero=true;
	;;
    *)
	echo "Usage: $progname [-options] MOUNTPOINT"
	echo 'Options: -c Content directory'
	echo "         -n Number of concurrent accesses (default: $nconcurrent)"
	echo '         -s Avoid staggerring start times'
	echo '         -z fill initial subvolume'
	exit 1
	;;
    esac
done

shift $(($OPTIND-1))
if [ $# -ne 1 ]; then
    echo "For usage: $progname -?"
    exit 1
fi

mountpoint=$1

echo 'Number of concurrent processes:' $nconcurrent
echo 'Content directory:' $content '(size:' `du -s $content | awk '{print $1}'` 'KB)'

if $fillzero; then
	mv $mountpoint/stress/0 $mountpoint/stress/0-$$-$RANDOM-to-delete
	echo "Create dir 0 as a snapshot source and import git"
	mkdir -p create $mountpoint/stress/ || true
	sudo $btrfs subvol create $mountpoint/stress/0
	echo "Copy .git and checkout"
	cp -a $content/.git $mountpoint/stress/0
	(
	 cd $mountpoint/stress/0
	 git rev-list --reverse master > rev-list
	 git checkout master
	)
fi

echo -n "Starting stress test processes: "

pids=""

p=1
while [ $p -le $nconcurrent ]; do
    echo -n "$p "
    prev=$(($p-1))

    (
	if [ $prev -gt 0 -a "$stagger" = "yes" ]; then
		while ! [ $mountpoint/stress/$prev/init-done ]; do
			sleep 10
		done
	fi

	echo -n "S$p "
	sudo $btrfs subvol snap $mountpoint/stress/$prev $mountpoint/stress/$p
	round=1

	cd $mountpoint/stress/$p

	shuf < rev-list | while read head; do
	    hdesc=$(git describe $head)
	    echo -n "G$p/$round/$hdesc "
	    git checkout -f $head
	    touch init-done
	    round=$(($round+1))
	done
    ) &

    pids="$pids $!"

    p=$(($p+1))
done

echo
echo "Process IDs: $pids"
echo "Press ^C to kill all processes"

trap "kill $pids" SIGINT

wait

kill $pids
