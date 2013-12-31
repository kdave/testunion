#!/usr/bin/perl

# decode various btrfs messages and try to print it in a human readable way

# currently:
# - transid verify failed
# - csum failed
# - relocating block group

my $out='';
my $all=1;
sub output {
	$out .= join(" ", @_);
	$out .= "\n";
}
sub popcount($) {
	my $maxbits=64;
	my $cnt=0;
	my $x=$_[0];
	for(my $i=0;$i<$maxbits;$i++) {
		$cnt++ if $x & (1<<$i);
	}
	return $cnt;
}
use constant BTRFS_BLOCK_GROUP_DATA	=>	(1 << 0);
use constant BTRFS_BLOCK_GROUP_SYSTEM	=>	(1 << 1);
use constant BTRFS_BLOCK_GROUP_METADATA	=>	(1 << 2);
use constant BTRFS_BLOCK_GROUP_RAID0	=>	(1 << 3);
use constant BTRFS_BLOCK_GROUP_RAID1	=>	(1 << 4);
use constant BTRFS_BLOCK_GROUP_DUP	=>	(1 << 5);
use constant BTRFS_BLOCK_GROUP_RAID10	=>	(1 << 6);
use constant BTRFS_AVAIL_ALLOC_BIT_SINGLE =>	(1 << 48);

while (($_=<>) ne '') {
	print if($all);
	# parent transid verify failed on 5083380932608 wanted 332337 found 339991
	if(/parent transid verify failed on (\d+) wanted (\d+) found (\d+)/) {
		output(sprintf("TRANSID: logical=%d wanted=%d(0x%x) found=%d(0x%x) delta=0x%x xorbits=%d",
			$1, $2, $2, $3, $3, int($2) ^ int($3), popcount(int($2) ^ int($3))));

	}
	# btrfs csum failed ino 66465 off 2752512 csum 1107022587 private 2566472073
	if(/csum failed ino (\d+) off (\d+) csum (\d+) private (\d+)/) {
		my $ino=$1;
		my $off=$2;
		my $csum=$3;
		my $priv=$4;
		output(sprintf("CSUM: ino=%d offset=%d(0x%x) csum=%d(0x%x) priv=0x%x xorbits=%d popcnt=%d",
			$ino, $off, $off, $csum, $csum, $priv, int($csum) ^ int($priv),
			popcount(int($csum) ^ int($priv))));
	}
	# btrfs csum failed ino 687504 extent 99728945152 csum 1248216615 wanted 2035797115 mirror -1703457408
	if(/csum failed ino (\d+) extent (\d+) csum (\d+) wanted (\d+) mirror (-?\d+)/) {
		my $ino=$1;
		my $ext=$2;
		my $csum=$3;
		my $want=$4;
		my $mirror=$5;
		output(sprintf("CSUM: ino=%d extent=%d(0x%x) csum=%d(0x%x) want=%d(0x%x) xorbits=%d popcnt=%d mirror=%d(0x%x)",
			$ino, $ext, $ext, $csum, $csum, $want, $want, int($csum) ^ int($want),
			popcount(int($2) ^ int($3)), $mirror, $mirror));
	}
	# relocating block group 1234 flags 45
	if(/relocating block group (\d+) flags (\d+)/) {
		my $bg=$1;
		my $fl=hex("0x$2");
		my $s='';
		my $single=1;
		$s.='DATA ' if($fl & BTRFS_BLOCK_GROUP_DATA);
		$s.='SYS ' if($fl & BTRFS_BLOCK_GROUP_SYSTEM);
		$s.='META ' if($fl & BTRFS_BLOCK_GROUP_METADATA);
		$single=0,$s.='RAID0 ' if($fl & BTRFS_BLOCK_GROUP_RAID0);
		$single=0,$s.='RAID1 ' if($fl & BTRFS_BLOCK_GROUP_RAID1);
		$single=0,$s.='RAID10 ' if($fl & BTRFS_BLOCK_GROUP_RAID10);
		$single=0,$s.='DUP ' if($fl & BTRFS_BLOCK_GROUP_DUP);
		$s.='SINGLE ' if($single);

		print("relocating bg $bg type $s\n");
	}
}

print("OUTPUT:\n$out\n");
