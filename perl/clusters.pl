#!/usr/bin/env perl
#!/usr/bin/env perl

use strict;
use warnings;

use File::Spec;

use GraphViz2;

use Log::Handler;

# ---------------

my($logger) = Log::Handler -> new;

$logger -> add
  (
   screen =>
   {
    maxlevel       => 'debug',
    message_layout => '%m',
    minlevel       => 'error',
   }
  );

my($graph) = GraphViz2 -> new
  (
   edge   => {color => 'grey'},
   global => {directed => 1},
   graph  => {label => 'Adult', rankdir => 'TB'},
   logger => $logger,
   node   => {shape => 'oval'},
  );

$graph->push_subgraph
  (
   name  => 'cluster_0',
   graph => {label => 'Child'},
   node  => {color => 'magenta', shape => 'diamond'},
   rank => "same",
  );
$graph->add_node(name => "a0");
$graph->add_node(name => "a1");
$graph->add_node(name => "a2");
$graph->add_node(name => "a3");

$graph->add_edge(from => "a0", to => "a1");
$graph->add_edge(from => "a1", to => "a2");
$graph->add_edge(from => "a2", to => "a3");

$graph->pop_subgraph;


$graph->push_subgraph
  (
   name  => 'cluster_1',
   graph => {label => 'Child'},
   node  => {color => 'magenta', shape => 'diamond'},
  );
$graph->add_node(name => "b0");
$graph->add_node(name => "b1", shape => "point");
$graph->add_node(name => "b2", shape => "point");
$graph->add_node(name => "b3", shape => "point");

$graph->add_edge(from => "b0", to => "b1");
$graph->add_edge(from => "b1", to => "b2");
$graph->add_edge(from => "b2", to => "b3");

$graph->pop_subgraph;

$graph->add_node(name => "start");
$graph->add_edge(from => "start", to => "a0");
$graph->add_edge(from => "start", to => "b0");
$graph->add_edge(from => "a1", to => "b3", weight => "0");


my($format)      = shift || 'svg';
my($output_file) = shift || File::Spec -> catfile('.', "clusters.graph.$format");

$graph -> run(format => $format, output_file => $output_file);


