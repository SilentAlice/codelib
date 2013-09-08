#!/usr/bin/env perl
use strict;
use Data::Dumper;
use v5.10;

local $/;
my $text = <>;

# my @cb = ($text =~ /(?:\n\n|\A)(?<cb>(?:(?:[ ]{4}|\t).*\n+)+)(?=^[ ]{0,4}\S|\z)/mg);
# above is my version.
# use /x to allow most whitespace and permit comments
# easy to understand
my @cb = ($text =~ m{
		      (?:\n\n|\A)          # $1 = the code block -- one or more lines, starting with a space/tab
		      (?<cb>
			(?:
			  (?:[ ]{4} | \t)  # Lines must start with a tab or a tab-width of spaces
			  .*\n+
			)+
		      )
		      (?:(?=^[ ]{0,4}\S)|\Z) # Lookahead for non-space at line-start, or end of doc
		  }gmx);

my @code_blocks = map {
  $_ =~ s/\A\n//g;
  $_ =~ s/\s+\z//g;
} @cb;

print Dumper @{cb};

