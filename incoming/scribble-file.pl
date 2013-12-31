#!/usr/bin/perl

use Fcntl;
my $file=$ARGV[0];

sysopen(F, $file, O_RDWR) or die;
my $fsize=-s $file;
my $blocks=int($fsize/4096);

my $count=$blocks*6;
my $sync_after=$blocks*5/100;

print("Open $file\n");
print("File size: $fsize\n");
print("File blocks: $blocks\n");
print("Start with $count iterations, sync every $sync_after\n");

while($count--) {
	#my $bi=4096*int(rand($blocks));
	my $bi=4096*int(rand($blocks));
	my $bo=int($bi/2);
	my $bdi;
	my $bdo;
	# read first block
	sysseek(F,$bi,0);
	sysread(F,$bdi,4096);
	# read second block
	sysseek(F,$bo,0);
	sysread(F,$bdo,4096);
	# write first over second
	sysseek(F,$bo,0);
	syswrite(F,$bdi,4096);
	# write second over first
	sysseek(F,$bi,0);
	syswrite(F,$bdi,4096);
	next;
	unless($sync_after--) {
		print("Sync, $count to go\n");
		system("fsync -- \"$file\"");
		$sync_after=$blocks*5/100;
	}
}
