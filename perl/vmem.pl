#!/usr/bin/env perl

use strict;
use warnings;
use 5.010;

use File::Spec;
use GraphViz2;
use Log::Handler;
use Data::Dumper;

use Getopt::Long::Descriptive;

################################################
# prepare parameters
################################################

my %arg;
my ($opt, $usage) = describe_options(
    'Usage: %c %o <some-arg>',
    [ 'action|a=s', "'dump' or 'draw'" ],
    [ 'format|f=s', "format used to read from binary files" ],
    [ 'verbose|v',  "print extra stuff"            ],
    [ 'help',       "print usage message and exit" ],
    \%arg
  );

print($usage->text), exit if $opt->help;
my $path = shift;
dumpbin($opt->format, $path);

sub dumpbin {
  my $format = shift;		# format to unpack the binary file
  my $path = shift;		# path of the file
  local $/ = \length(pack($format, 0)); # test the number of bytes of an entry

  open FILE, "<", $path or die "Fail to open binary file: $path";
  while (<FILE>) {
    my @entry = unpack $format;
    # say join " ", ($a, $b, $c);
    say join " ", @entry;
  }
  close FILE;
}

################################################
# read file
################################################

my $nthreads = 2;

# open MEMOP, "<", "/Volumes/styx/zion/drreplay/build/log/mem-op-1" or die "Can't open binary file";
# open MEMVER, "<", "/Volumes/styx/zion/drreplay/build/log/mem-version-1" or die "Can't open binary file";
# binmode(MEMOP);
# binmode(MEMVER);
# $/ = \24;

# my %wait_ver;
# while (<MEMOP>) {
#   my ($objid, $version, $memop) = unpack "q q2";
#   $/ = \16;
#   my $memversion = <MEMVER>;
#   my ($nextmemop, $nextversion) = unpack "q2", $memversion;
#   $wait_ver{$objid}{$version}{1}{"last"} = $memop;
#   $wait_ver{$objid}{$version}{1}{"next"}{"memop"} = $nextmemop;
#   $wait_ver{$objid}{$version}{1}{"next"}{"version"} = $nextversion;
#   $/ = \24;
# }

# # print Dumper \%wait_ver;

# close MEMOP;
# close MEMVER;

# open MEMOP, "<", "/Volumes/styx/zion/drreplay/build/log/mem-op-2" or die "Can't open binary file";
# open MEMVER, "<", "/Volumes/styx/zion/drreplay/build/log/mem-version-2" or die "Can't open binary file";
# binmode(MEMOP);
# binmode(MEMVER);
# $/ = \24;

# while (<MEMOP>) {
#   my ($objid, $version, $memop) = unpack "q q2";
#   $/ = \16;
#   my $memversion = <MEMVER>;
#   my ($nextmemop, $nextversion) = unpack "q2", $memversion;
#   $wait_ver{$objid}{$version}{2}{"last"} = $memop;
#   $wait_ver{$objid}{$version}{2}{"next"}{"memop"} = $nextmemop;
#   $wait_ver{$objid}{$version}{2}{"next"}{"version"} = $nextversion;
#   $/ = \24;
# }

# print Dumper \%wait_ver;

# close MEMOP;
# close MEMVER;

# ##################################################
# # draw graph
# ##################################################

# my($logger) = Log::Handler -> new;

# $logger -> add
#   (
#    screen =>
#    {
#     maxlevel       => 'debug',
#     message_layout => '%m',
#     minlevel       => 'error',
#    }
#   );

# my($graph) = GraphViz2 -> new
#   (
#    edge   => {color => 'grey'},
#    global => {directed => 1},
#    graph  => {label => 'Adult', rankdir => 'TB'},
#    logger => $logger,
#    node   => {shape => 'oval'},
#   );

# print Dumper $wait_ver{6148};

# my ($last_1, $last_2, $last_6148);
# foreach my $entry (sort {$a <=> $b} keys $wait_ver{6148}) {
#   $graph->push_subgraph(
# 			name=> 'cluster_6148',
# 		       );
#   $graph->add_node(name => '6148_' . $entry);
#   $graph->add_edge(from => $last_6148, to => "6148_" . $entry) unless (undef == $last_6148);
#   $last_6148 = "6148_" . $entry;
#   $graph->pop_subgraph;

#   if (undef != $wait_ver{6148}{$entry}{1}) {
#     $graph->push_subgraph(
# 			  name => 'cluster_1',
# 			 );
#     $graph->add_node(name => "1_" . $entry);
#     $graph->add_edge(from => $last_1, to => "1_" . $entry) unless (undef == $last_1);
#     $graph->add_node(name => "1_" . $wait_ver{6148}{$entry}{1}{'next'}{'version'});
#     $graph->add_edge(from => "1_" . $entry, to => "1_" . $wait_ver{6148}{$entry}{1}{'next'}{'version'});
#     $last_1 = "1_" . $wait_ver{6148}{$entry}{1}{'next'}{'version'};
#     $graph->add_edge(from => "1_" . $entry, to => "6148_" . $entry, weight => 0);
#     $graph->pop_subgraph;
#   }
#   if (undef != $wait_ver{6148}{$entry}{2}) {
#     $graph->push_subgraph(
# 			  name => 'cluster_2',
# 			 );
#     $graph->add_node(name => "2_" . $entry);
#     $graph->add_edge(from => $last_2, to => "2_" . $entry) unless (undef == $last_2);
#     $graph->add_node(name => "2_" . $wait_ver{6148}{$entry}{2}{'next'}{'version'});
#     $graph->add_edge(from => "2_" . $entry, to => "2_" . $wait_ver{6148}{$entry}{2}{'next'}{'version'});
#     $last_2 = "2_" . $wait_ver{6148}{$entry}{2}{'next'}{'version'};
#     $graph->add_edge(from => "2_" . $entry, to => "6148_" . $entry, weight => 0);
#     $graph->pop_subgraph;
#   }
# }


# $graph -> default_node(color => 'cyan');

# my($format)      = shift || 'svg';
# my($output_file) = shift || File::Spec -> catfile('.', "visual.graph.$format");

# $graph -> run(format => $format, output_file => $output_file);
