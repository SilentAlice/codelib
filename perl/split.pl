#!/usr/bin/env perl

use strict;
use Data::Dumper;

# from Markdown.pl
# string to array
use Digest::MD5 qw(md5_hex);
my %g_escape_table;
foreach my $char (split //, '\\`*_{}[]()>#+-.!') {
	$g_escape_table{$char} = md5_hex($char);
}

print Dumper \%g_escape_table;

# test existence of a package and use as a condition
eval {require MT};
print Dumper $@;
unless ($@) {
} else {
}

my $str = "world";
# print "$1\n" if ($str =~ m/(?:worlo)/);

print "$1\n" if ($str =~ /^(?:world)/);
