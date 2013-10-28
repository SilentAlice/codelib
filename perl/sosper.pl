#!/usr/bin/env perl

use strict;
use Data::Dumper;

open DATA, "<", shift || die "fail to open data file";

my @attendee_list;
my $cnum = 0;

my %colums = (
	      "0" => "id",
	      "1" => "name",
	      "2" => "company",
	      "3" => "country",
	      "4" => "regtype",
	      "5" => "events",
	      "6" => "workshop"
	     );

sub read_data {
  while (<DATA>) {
    s/(\r?\n?)$//g;		  # remove trailing newline
    my @values = split("\t", $_, -1); # split values
    print scalar(@values), "\n";
    # print @values, "\n";
    # while (scalar @values != 7) { # fill empty values
    #   push @values, "";
    # }
    my %hash = map { get_column_name() => $_ } @values; # transform to hash
    # print Dumper \%hash;
    # print "\n";
    push @attendee_list, \%hash; # why must "\" here?
  }
}

sub get_column_name {
  # print $cnum, "\n";
  my $result = $colums{$cnum};
  $cnum = ($cnum + 1) % 7;
  return $result;
}

read_data();

# for $entry in 
print Dumper \@attendee_list;

close DATA;
