#!/usr/bin/perl
#
# usage: $0 <path> [find args...]

use warnings;
use strict;
$!++;

my $path = shift @ARGV;
die "Path $path not a directory\n" if (! -d $path);
$path = `readlink -f "$path"`;
chdir($path);
print("Path: $path\n");

my $last = time;
my %hist=();

# paths prefixed with ./
open(F, "find . @ARGV |") or die $!;
my $f;
while(($f=<F>) ne '') {
	#print;
	chomp $f;
	next if($f eq "");
	next if($f eq ".");
	next if($f eq "..");
	next if($f eq "./");
	next if($f eq "../");
	my $l=length($f) - length("./");

	if (time - $last >= 1) {
		print("At: $f\n");
		$last=time;
	}
	print "LEN $l $hist{$l} $f\n";
	if (defined $hist{$l}) {
		$hist{$l}++;
	} else {
		$hist{$l}=1;
	}
}
close(F);

print("# length    count\n");
foreach(sort { $b <=> $a } keys %hist) {
	printf("% 8d % 8d\n", $_, $hist{$_});
}

