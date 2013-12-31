#!/bin/bash -
# -*- Shell-script -*-
#
# simulate filesystem aging in subvolumes by git activity

nconcurrent=8
content=/mnt/aoe1/wikidump
mountpoint=/mnt/aoe1
fillzero=false
progname=`basename "$0"`
btrfs='sudo btrfs'

while getopts "c:n:sz" c; do
    case $c in
    c) content=$OPTARG ;;
    n) nconcurrent=$OPTARG ;;
    m) mountpoint=$OPTARG ;;
    z) fillzero=true ;;
    *)
	echo "Usage: $progname [-cmnz]"
	echo 'Options: -c Content directory'
	echo '         -m mountpoint'
	echo "         -n Number of concurrent accesses (default: $nconcurrent)"
	echo '         -z fill initial subvolume'
	exit 1
	;;
    esac
done

if ! [ -d $mountpoint ]; then
	echo "Mountpoint $mountpoint does not exist"
	exit 1
fi
if ! [ -d $content ]; then
	echo "Content directory $content does not exist"
	exit 1
fi

if ! $btrfs; then
	echo "Not able to run '$btrfs'"
	exit 1
fi

echo 'Number of concurrent processes:' $nconcurrent
echo -n "Calculating content dir size... "
echo ':' $content '(size:' `du -s $content | awk '{print $1}'` 'KB)'

if $fillzero; then
	echo "Create dir 0 as a snapshot source and import git"
	if [ -d $mountpoint/stress/0 ]; then
		echo "Found old 0 dir, trying to remove"
		rmdir $mountpoint/stress/0 ||
			$btrfs subvol delete $mountpoint/stress/0 ||
			mv $mountpoint/stress/0-$$-$RANDOM-to-delete
	fi
	mkdir -p create $mountpoint/stress/ || true
	$btrfs subvol create $mountpoint/stress/0
	echo "Copy .git and checkout"
	cp -a $content/.git $mountpoint/stress/0
	(
	 cd $mountpoint/stress/0
	 git checkout -f master
	)
fi

echo -n "Starting stress test processes: "
pids=""
p=1
while [ $p -le $nconcurrent ]; do
    echo -n "$p "
    prev=$(($p-1))

    (
	echo -n "S$p "
	$btrfs subvol snap $mountpoint/stress/$prev $mountpoint/stress/$p
	rm -f $mountpoint/stress/$p/.git/index.lock >&/dev/null
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
