#!/bin/bash -
# -*- Shell-script -*-
#
# Copyright (C) 1999 Bibliotech Ltd., 631-633 Fulham Rd., London SW6 5UQ.
#
# $Id: stress.sh,v 1.2 1999/02/10 10:58:04 rich Exp $
#
# Change log:
#
# $Log: stress.sh,v $
# Revision 1.2  1999/02/10 10:58:04  rich
# Use cp instead of tar to copy.
#
# Revision 1.1  1999/02/09 15:13:38  rich
# Added first version of stress test program.
#

# Stress-test a file system by doing multiple
# parallel disk operations. This does everything
# in MOUNTPOINT/stress.

nconcurrent=50
content=/usr/share/doc
stagger=yes

while getopts "c:n:s" c; do
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
    *)
	echo 'Usage: stress.sh [-options] MOUNTPOINT'
	echo 'Options: -c Content directory'
	echo "         -n Number of concurrent accesses (default: $nconcurrent)"
	echo '         -s Avoid staggerring start times'
	exit 1
	;;
    esac
done

shift $(($OPTIND-1))
if [ $# -ne 1 ]; then
    echo 'For usage: stress.sh -?'
    exit 1
fi

mountpoint=`cd $1;pwd`

echo 'Number of concurrent processes:' $nconcurrent
echo 'Content directory:' $content '(size:' `du -s $content | awk '{print $1}'` 'KB)'

# Check the mount point is really a mount point.

#if [ `df | awk '{print $6}' | grep ^$mountpoint\$ | wc -l` -lt 1 ]; then
#    echo $mountpoint: This doesn\'t seem to be a mountpoint. Try not
#    echo to use a trailing / character.
#    exit 1
#fi

# Create the directory, if it doesn't exist.

if [ ! -d $mountpoint/stress ]; then
    rm -rf $mountpoint/stress
    if ! mkdir $mountpoint/stress; then
	echo Problem creating $mountpoint/stress directory. Do you have sufficient
	echo access permissions\?
	exit 1
    fi
fi

echo Created $mountpoint/stress directory.

# Construct MD5 sums over the content directory.

echo -n "Computing MD5 sums over content directory: "
(
 cd $content &&
 find . -type f -print0 |
 xargs -0 md5sum |
 sort > $mountpoint/stress/content.sums )
echo done.

# Start the stressing processes.

echo -n "Starting stress test processes: "

pids=""

p=1
while [ $p -le $nconcurrent ]; do
    echo -n "$p "

    (

	# Wait for all processes to start up.
	if [ "$stagger" = "yes" ]; then
	    sleep $((10*$p))
	else
	    sleep 10
	fi

	while true; do

	    # Remove old directories.
	    echo -n "D$p "
	    rm -rf $mountpoint/stress/$p

	    # Copy content -> partition.
	    echo -n "W$p "
	    mkdir $mountpoint/stress/$p
	    base=`basename $content`

	    #( cd $content && tar cf - . ) | ( cd $mountpoint/stress/$p && tar xf - )
	    cp -dRx $content $mountpoint/stress/$p

	    # Compare the content and the copy.
	    echo -n "R$p "
	    ( cd $mountpoint/stress/$p/$base && find . -type f -print0 | xargs -0 md5sum | sort -o /tmp/stress.$$.$p )
	    diff $mountpoint/stress/content.sums /tmp/stress.$$.$p
	    if [ $? != 0 ]; then
	        echo "file miscompares in $p"
		killall stress.sh
		exit 1
	    fi
	    rm -f /tmp/stress.$$.$p
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
