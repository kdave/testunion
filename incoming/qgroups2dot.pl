#!/usr/bin/perl

# convert btrfs qgroup hierarchy to graphviz format
# 'btrfs qgroup -c /path'

sub id2n($) {
	my $id=@_[0];
	$id =~ s,/,x,;
	return "n" . $id;
}

my $level0='';
my $edges='';
my $rest='';

while(<>) {
	chomp;
	print STDERR "Processing $_\n";
	my @l = split(/\s+/);
	my $shape = '';
	next if($l[0] !~ '\d+/\d+');
	if($l[3] ne '---') { # middle node
		foreach my $c (split(",", $l[3])) {
			$edges .= id2n($l[0]) . " -> " . id2n($c) . ";\n";
		}
	}
	if($l[0] =~ "^0/") {
		$shape = "shape=box,";
	}
	$l[1]=`./pretty $l[1]`;
	$l[2]=`./pretty $l[2]`;
	chomp $l[1];
	chomp $l[2];
	$line = id2n($l[0]) . " [${shape}label=\"$l[0]\\nR=$l[1]\\nE=$l[2]\"];\n";

	if($l[0] =~ "^0/") {
		$level0.=$line;
	} else {
		$rest.=$line;
	}
}
print "digraph {\n";
print "subgraph cluster_level0 {\n";
print "shape=none;\n";
print "color=none;\n";
print "$level0\n";
print "}\n";
print "$rest\n";
print "$edges\n";
print "}";
