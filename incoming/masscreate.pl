#!/usr/bin/perl

use strict;
use warnings;
use threads;
use Fcntl;

my $size=3900;
my $fillbuf='A'x $size;

sub worker {
	my ($id,$start,$step)=@_;
	print("Start $start, step $step\n");
	my $yc=100;	# yield counter
	my $y=$yc;
	while(1) {
		print("[$id] $start\n");
		sysopen(FH, $start, O_TRUNC | O_WRONLY | O_NONBLOCK | O_CREAT, 0644);
		syswrite(FH, $fillbuf, $size) if($size);
		close(FH);
		$start+=$step;
		if(!$y--) {
			threads->yield();
			$y=$yc;
		}
	}
}

my $i;
my $start=0;
my $workers=4;

if($ARGV[0] =~ /^\d+$/) {
	$start=$ARGV[0];
}
print("Start at $start\n");
print("Size: $size\n");

for($i=1;$i<=$workers;$i++) {
	threads->create('worker', $i, $start + $i - 1, $workers);
}
while(threads->list()) {
	foreach(threads->list(threads::joinable)) {
		print("Join ". $_->tid() ."\n");
		$_->join();
	}
}
