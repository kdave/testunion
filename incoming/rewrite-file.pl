#!/usr/bin/perl

use Fcntl;
my $file=$ARGV[0];

sysopen(F, $file, O_RDWR) or die;
my $fsize=-s $file;
my $blocks=int($fsize/4096);

my $count=$blocks*10;
my $sync_after=int($blocks*10/100);
my $no_sync=1;

print("Open $file\n");
print("File size: $fsize\n");
print("File blocks: $blocks\n");
print("Start with $count iterations, sync every $sync_after\n") unless($no_sync);

my $tick=time;

while($count--) {
	my $bi=4096*int(rand($blocks));
	my $bdi;
	sysseek(F,$bi,0);
	sysread(F,$bdi,4096);
	sysseek(F,$bi,0);
	syswrite(F,$bdi,4096);
	unless($no_sync) {
		if($sync_after-- == 0) {
			print("Sync, $count to go\n");
			system("fsync \"$file\"");
			$sync_after=int($blocks*10/100);
		}
		my $now=time;
		if($now - $tick >= 1) {
			print("At $count, sync $sync_after\n");
			$tick=$now;
		}
	}
}
