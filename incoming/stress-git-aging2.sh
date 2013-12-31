#!/bin/bash -
# -*- Shell-script -*-
#
# simulate filesystem aging in subvolumes by git activity

nconcurrent=8
#content=/mnt/aoe1/kernel-upstream
content=/mnt/aoe1/btrfs-gui
mountpoint=/mnt/aoe1
fillzero=true
progname=`basename "$0"`
btrfs='sudo btrfs'
maxrounds=10

function msg() {
	ts=`date +%H:%M:%S`
	echo "*** $ts: $@"
}

while getopts "c:n:r:z" c; do
    case $c in
    c) content=$OPTARG ;;
    n) nconcurrent=$OPTARG ;;
    m) mountpoint=$OPTARG ;;
    r) maxrounds=$OPTARG ;;
    z) fillzero=true ;;
    *)
	echo "Usage: $progname [options]"
	echo 'Options: -c Content directory'
	echo '         -m mountpoint'
	echo "         -n Number of concurrent accesses (default: $nconcurrent)"
	echo "         -z fill initial subvolume (default: $fillzero)"
	echo "         -r maximum rounds per snapshot chain (default: $maxrounds)"
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

if ! $btrfs >& /dev/null; then
	echo "Not able to run '$btrfs'"
	exit 1
fi

msg 'Number of concurrent processes:' $nconcurrent
msg "Calculating content dir size: $content (`du -s $content | awk '{print $1}'` 'KB)"

mkdir -p $mountpoint/stress/ || true

if $fillzero; then
	msg "Create dir 0 as a snapshot source and import git"
	if [ -d $mountpoint/stress/0 ]; then
		msg "Found old 0 dir, trying to remove"
		rmdir $mountpoint/stress/0 ||
			$btrfs subvol delete $mountpoint/stress/0 ||
			mv $mountpoint/stress/0 $mountpoint/stress/0-$$-$RANDOM-to-delete
	fi
	$btrfs subvol create $mountpoint/stress/0
	msg "Copy .git and checkout"
	cp -a $content/.git $mountpoint/stress/0
	(
	 cd $mountpoint/stress/0
	 git rev-list --reverse master > rev-list
	 git checkout -f master
	)
fi

msg "Starting stress test processes: "
pids=""
p=1
while [ $p -le $nconcurrent ]; do
    msg "Start $p and init 0 snapshot $p/0 "
    round=1
    prev=0
    d=$p-0
    $btrfs subvol snap $mountpoint/stress/0 $mountpoint/stress/$d
    rm -f $mountpoint/stress/$d/.git/index.lock >&/dev/null
    (
	shuf < $mountpoint/stress/0/rev-list | while read head; do
		msg "Snapshot $p/$round "
		dp=$d
		d=$p-$round
		$btrfs subvol snap $mountpoint/stress/$dp $mountpoint/stress/$d
		rm -f $mountpoint/stress/$d/.git/index.lock >&/dev/null

		(
		 cd $mountpoint/stress/$d

		 hdesc=$(git describe $head)
		 # no tags?
		 if [ $? = 128 ]; then
			hdesc=$head
		 fi
		 msg "Git $p/$round/$hdesc"
		 git checkout -f $head
		 touch init-done
		)
		prev=$round
		round=$(($round+1))
		if [ $round -gt $maxrounds ]; then
			break
		fi
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
