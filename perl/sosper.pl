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
    # print scalar(@values), "\n";
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

sub apply_filter {
  # my $result = shift->(shift);
  my $func = shift;
  my $arg = shift;
  my $result = $func->($arg);
  last if ($result == 0);
}

sub filter_SOSP {
  # print Dumper shift;
  my %income = shift;
  # my $inp = shift;
  my $sosp = $income{'events'};
  print Dumper \%income;
  if ($sosp =~ m/SOSP/ig) {
    return 1;
  } else {
    return 0;
  }
}

read_data();

for my $entry (@attendee_list) {
  apply_filter(\&filter_SOSP, $entry);
  print Dumper $entry;
  print "\n";
}

# for $entry in 
# print Dumper \@attendee_list;

close DATA;
