#!/bin/bash -
# -*- Shell-script -*-
#
# simulate filesystem aging by git activity

nconcurrent=4
content=~/DATA_SOURCES/glibc-git
mountpoint=/mnt/test
progname=`basename "$0"`

function msg() {
	ts=`date +%H:%M:%S`
	echo "*** $ts: $@"
}

while getopts "c:n:z" c; do
    case $c in
    c) content=$OPTARG ;;
    n) nconcurrent=$OPTARG ;;
    m) mountpoint=$OPTARG ;;
    *)
	echo "Usage: $progname [options] taglist..."
	echo 'Options: -c Content directory'
	echo '         -m mountpoint'
	echo "         -n Number of concurrent accesses (default: $nconcurrent)"
	exit 1
	;;
    esac
done

shift $((OPTIND-1))
if [ "$#" = 0 ]; then
	echo "Taglist not specified"
	exit 1
fi

taglist=( $@ )


if ! [ -d $mountpoint ]; then
	echo "Mountpoint $mountpoint does not exist"
	exit 1
fi
if ! [ -d $content ]; then
	echo "Content directory $content does not exist"
	exit 1
fi

msg 'Number of concurrent processes:' $nconcurrent

mkdir -p $mountpoint/git-aging/ || true

msg "Copy .git and checkout initial dir"
mkdir $mountpoint/git-aging/0
cp -a $content/.git $mountpoint/git-aging/0/.git
pwd="`pwd`"
cd $mountpoint/git-aging/0
git checkout -q -f master
cd "$pwd"

msg "Starting git-aging test processes: "
pids=""
p=1
while [ $p -le $nconcurrent ]; do
    msg "Start $p "
    (
	d=$p
	msg "Copy 0 to $d"
	cp -a $mountpoint/git-aging/0 $mountpoint/git-aging/$d
	rm -f $mountpoint/git-aging/$d/.git/index.lock >&/dev/null

	prevhead=master
    	for head in ${taglist[@]}; do
		(
		 cd $mountpoint/git-aging/$d

		 msg "Git $p/$head"
		 git checkout -q -f -b test-$head $head
		 msg "Git $p/$head read ops"
		 git diff $prevhead >& /dev/null
		 git log >& /dev/null
		 git grep 'x' >& /dev/null
		)
		prevhead=$head
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

kill $pids >&/dev/null
