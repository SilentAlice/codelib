#!/usr/bin/perl
use Data::Dumper;

$interval = $ARGV[1];
$total_count = 0;
$max = 0;
$min = 0xffffffff;

open RAW, "<", $ARGV[0] or die "fail to open file: $ARGV[0]";

while (<RAW>) {
	$data[$_/$interval]++;
	$total_count++;
	if ($_ > $max) {
		$max = $_;
	}
	if ($_ < $min) {
		$min = $_;
	}
}

# foreach $i (1 .. $#data) {
	# $data[$i] += $data[$i-1];
# }

foreach $i (0 .. $#data) {
	$cum = $data[$i] / $total_count;
	print $i*$interval, "\t", $cum, "\n";
}
