#!/usr/bin/env perl

@range = ("A".."Z", 0..9);	# range of characters
my $length = 16;		# length of the code
my $count = 8;			# count of codes

foreach (1..$count) {
  print join "", @range[ map { rand @range } 1..16 ], "\n"
}
