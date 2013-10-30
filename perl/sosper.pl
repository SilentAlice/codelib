#!/usr/bin/env perl
use 5.010;

use strict;
use Data::Dumper;
use Locale::Object::Continent;

open DATA, "<", shift || die "fail to open data file";

my %country_hash;
my %continent_cnt;

my @attendee_list;
my $cnum = 0;

my %workshop_cnt;
my $sosp_cnt = 0;
my $total_cnt = 0;

# regtype breakdown
my %regtype_cnt;
my $regtype_student = 0;
my $regtype_nonstudent = 0;

# geographic breakdown
my %geo_cnt;

my %colums = (
	      "0" => "id",
	      "1" => "name",
	      "2" => "company",
	      "3" => "country",
	      "4" => "regtype",
	      "5" => "events",
	      "6" => "workshop",
	      "7" => "charge"
	     );

my $student_academics;
my $nonstudent_academics;
my $industry;
my $other;
my %company_list = (
		"AT&T Research Labs" => "0",
		"CNRS - IRIT - ENSEEIHT" => "0",
		"Confreaks" => "0",
		"Ecole Polytechnique de Montreal" => "0",
		"FAU Erlangen-Nuremberg" => "0",
		"FCT/UNL" => "0",
		"Fermilab" => "0",
		"Fujitsu Laboratories Ltd." => "0",
		"HGST Research" => "0",
		"HP Labs" => "0",
		"HP Labs - Palo Alto" => "0",
		"IBM Research" => "0",
		"IBM Research - Zurich" => "0",
		"IBM T. J. Watson Research Center" => "0",
		"Indian institute of Science" => "0",
		"INESC-ID" => "0",
		"INRIA / UPMC-LIP6" => "0",
		"INRIA LIP6" => "0",
		"INRIA-LIP6" => "0",
		"Institute of Computing Technology, CAS" => "0",
		"Institute of Software, Chinese Academy of Sciences" => "0",
		"Intel Labs" => "0",
		"Kaspersky Lab" => "0",
		"LIP6/INRIA" => "0",
		"Max Planck Institute for Software Systems" => "0",
		"Microsoft Research" => "0",
		"Microsoft Research - Silicon Valley" => "0",
		"Microsoft Research (SVC)" => "0",
		"Microsoft Research Asia" => "0",
		"Microsoft Research Cambridge" => "0",
		"Microsoft Research Silicon Valley" => "0",
		"MPI-SWS" => "0",
		"NEC Laboratories America" => "0",
		"NEC Labs" => "0",
		"NEC Labs America" => "0",
		"NICTA and UNSW" => "0",
		"Oak Ridge National Lab (ORNL)" => "0",
		"Oracle Labs" => "0",
		"Qatar Computing Research Institute" => "0",
		"Shenzhen Institute of Information Technology" => "0",
		"Symantec Research Labs" => "0",
		"Systems Group / ETH Zurich" => "0",
		"Systems Group, ETH Zurich" => "0",
		"Technion" => "0",
		"Technion (EE)" => "0",
		"Yahoo Research" => "0",
		"Amherst College" => "1",
		"Beihang University" => "1",
		"Boston University" => "1",
		"Brandeis University" => "1",
		"BU" => "1",
		"Carnegie Mellon University" => "1",
		"CITI - New University of Lisbon" => "1",
		"City University London" => "1",
		"Clarkson University" => "1",
		"Columbia University" => "1",
		"Cornell" => "1",
		"Cornell University" => "1",
		"Denison University" => "1",
		"Dickinson College" => "1",
		"Duke University" => "1",
		"EPFL" => "1",
		"ETH Zurich" => "1",
		"ETH Zürich" => "1",
		"Georgia Institute of Technology" => "1",
		"Georgia Tech" => "1",
		"Hanyang University" => "1",
		"Hanyang Univrsity" => "1",
		"Harvard University" => "1",
		"Harvey Mudd College" => "1",
		"Hongik University" => "1",
		"Indian Institute of Technology Delhi (IIT Delhi)" => "1",
		"Johns Hopkins University" => "1",
		"Keio Univ." => "1",
		"Keio University" => "1",
		"McGill University" => "1",
		"MIT" => "1",
		"MIT CSAIL" => "1",
		"New University of Lisbon" => "1",
		"New York University" => "1",
		"NICTA" => "1",
		"North Carolina State University" => "1",
		"Northeastern University" => "1",
		"NYU" => "1",
		"NYU and UT Austin" => "1",
		"Peking University" => "1",
		"Politecnico di Milano" => "1",
		"Princeton University" => "1",
		"Purdue University" => "1",
		"Reykjavik University" => "1",
		"Rice University" => "1",
		"Royal Holloway, University of London" => "1",
		"Rutgers University" => "1",
		"Shanghai Jiao Tong University" => "1",
		"Shanghai JiaoTong University" => "1",
		"Singapore Management University" => "1",
		"Stanford" => "1",
		"Stanford University" => "1",
		"Stony Brook" => "1",
		"Stony Brook University" => "1",
		"SUNY Buffalo" => "1",
		"SUNY Stony Brook" => "1",
		"Technische Universität Dresden" => "1",
		"The College of William and Mary" => "1",
		"The Ohio State University" => "1",
		"The University of Texas at Austin" => "1",
		"The University of Tokyo" => "1",
		"Tsinghua University" => "1",
		"TU Braunschweig" => "1",
		"TU Dortmund" => "1",
		"TU Dresden" => "1",
		"TUAT" => "1",
		"Tufts University" => "1",
		"U. Nova de Lisboa" => "1",
		"UBC Computer Science" => "1",
		"UC Berkeley" => "1",
		"UC Riverside" => "1",
		"UC San Diego" => "1",
		"UCSD" => "1",
		"Univ. of Tromsø" => "1",
		"University College London (UCL)" => "1",
		"University of Arizona" => "1",
		"University of British Columbia" => "1",
		"University of British Columbia, Vancouver" => "1",
		"University of California San Diego" => "1",
		"University of California, Riverside" => "1",
		"University of California, San Diego" => "1",
		"University of Cambridge" => "1",
		"University of Central Florida" => "1",
		"University of Chicago" => "1",
		"University of Hyogo" => "1",
		"University of Illinois at Chicago" => "1",
		"University of Illinois at Urbana Champaign" => "1",
		"University of Illinois at Urbana-Champaign" => "1",
		"University of Mannheim" => "1",
		"University of Massachusetts, Amherst" => "1",
		"University of Michigan" => "1",
		"University of Nice Sophia-Antipolis" => "1",
		"University of Pennsylvania" => "1",
		"University of Seoul" => "1",
		"University of Southern California" => "1",
		"University of Stavanger" => "1",
		"University of Texas at Austin" => "1",
		"University of Toronto" => "1",
		"University of Tromsø" => "1",
		"University of Tsukuba" => "1",
		"University of Utah" => "1",
		"University of Washington" => "1",
		"University of Waterloo" => "1",
		"University of Wisconsin, Madison" => "1",
		"University of Wisconsin-Madison" => "1",
		"Univesity of Tokyo" => "1",
		"UPCM/LIP6" => "1",
		"USP" => "1",
		"UT Austin" => "1",
		"UT Austin/Google" => "1",
		"Virginia Tech" => "1",
		"VU University" => "1",
		"VU University Amsterdam" => "1",
		"Wheaton College (IL)" => "1",
		"Xi'an Jiaotong University" => "1",
		"Yale" => "1",
		"Yale University" => "1",
		"Akamai Technologies" => "2",
		"Amazon EC2" => "2",
		"Amazon Web Services" => "2",
		"Apple" => "2",
		"Apple Inc" => "2",
		"Apple Inc." => "2",
		"Apple, Inc." => "2",
		"Apportable" => "2",
		"BAE Systems" => "2",
		"Baidu" => "2",
		"baidu, inc" => "2",
		"BAIDU,INC." => "2",
		"Boeing" => "2",
		"Bromium UK Ltd" => "2",
		"Cisco" => "2",
		"Cisco Systems" => "2",
		"Data Storage Institute, A*STAR" => "2",
		"EMC" => "2",
		"EMC Backup Recovery Systems" => "2",
		"Facebook" => "2",
		"Facebook Inc" => "2",
		"Facebook Inc." => "2",
		"Facebook, Inc" => "2",
		"Galois, Inc" => "2",
		"Google" => "2",
		"Google Inc." => "2",
		"Google, Inc." => "2",
		"Hitachi, Ltd." => "2",
		"HP Vertica" => "2",
		"HP/Vertica" => "2",
		"Huawei Inc" => "2",
		"HUAWEI TECHNOLOGIES CO., LTD" => "2",
		"Huawei technologies co., ltd." => "2",
		"IMDEA Networks" => "2",
		"Intel Corp" => "2",
		"Intel Corporation" => "2",
		"Microsoft" => "2",
		"Microsoft (CISL)" => "2",
		"Microsoft Corporation" => "2",
		"Nessos Information Technologies SA" => "2",
		"NetApp" => "2",
		"NetApp Inc." => "2",
		"NetApp, Inc." => "2",
		"Nutanix Inc" => "2",
		"Parallels" => "2",
		"POSTECH" => "2",
		"Riverbed" => "2",
		"Salesforce.com" => "2",
		"Samsung Electronics" => "2",
		"Samsung Electronics co., Ltd." => "2",
		"Sony Corporation" => "2",
		"Twitter" => "2",
		"Twitter Inc" => "2",
		"Twitter, Inc." => "2",
		"Vertica" => "2",
		"VMware" => "2",
		"VMware, Inc" => "2",
		"VMware, Inc." => "2",
		"National Science Foundation" => "3",
		"New Mexico Consortium" => "3",
		"NSA" => "3",
		"NSF" => "3",
		"Xu" => "3",
		"Yandex" =>"3"
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
  $cnum = ($cnum + 1) % scalar keys %colums;
  return $result;
}

sub dump_hash {
  my $hash = shift;
  keys $hash;
  while (my($k,$v) = each $hash) {
    print "$k\t$v\n";
  }
}

sub prepare_country {
  my @continent_list = ("Asia", "Europe", "North America");

  map {
    my $continent = Locale::Object::Continent->new( name => $_ );
    my @continent_countries;
    foreach my $c ($continent->countries) {
      push @continent_countries, $c->name;
    }
    $country_hash{$_} = \@continent_countries;
  } @continent_list;
}

# apply filter to defined filter subroutines
# subroutines should return 0 for skip the current entry and
# 1 for accept the entry
sub apply_filter {
  # my $result = shift->(shift);
  my $func = shift;
  my $arg = shift;
  my $result = $func->($arg);
  next if ($result == 0);
}

sub filter_SOSP {
  my $income = shift;		  # `income' is also a reference
  my $sosp = $income->{'events'}; # so `$income{'events'} is wrong
  my $workshop = $income->{'workshop'};

  # test if the events contains SOSP
  if ($sosp =~ m/SOSP/ig || $sosp eq "") {
    return 1;
  } else {
    return 0;
  }
}

sub filter_workshop {
  my $income = shift;
  my $workshop = $income->{'workshop'};

  if ($workshop =~ m/Diversity|HotDep|HotPower|INFLOW|LADIS|PLOS|TRIOS/ig) {
    $workshop_cnt{$workshop}++;
    return 1;
  } else {
    return 0;
  }
}

sub filter_regtype {
  my $income = shift;
  my $regtype = $income->{'regtype'};

  if ($regtype =~ m/student/ig) {
    $regtype_student++;
  } else {
    $regtype_nonstudent++;
  }

  $regtype_cnt{$regtype}++;
}

sub filter_geo {
  my $income = shift;
  my $country = $income->{'country'};

  $geo_cnt{$country}++;

  if (grep { $country eq $_ } @{$country_hash{'Asia'}}) {
    $continent_cnt{'Asia'}++;
  } elsif (grep { $country eq $_ } @{$country_hash{'Europe'}}) {
    $continent_cnt{'Europe'}++;
  } elsif (grep { $country eq $_ } @{$country_hash{'North America'}}) {
    $continent_cnt{'North America'}++;
  } else {
    $continent_cnt{'Other'}++;
  }

}

sub filter_company {
  my $income = shift;
  my $regtype = $income->{'regtype'};
  my $company = $income->{'company'};

  if ($regtype =~ m/student/ig) {
    $student_academics++;
  } elsif ($company_list{$company} == 1 || $company_list{$company} == 0) {
    $nonstudent_academics++;
  } elsif ($company_list{$company} == 2) {
    $industry++;
  } else {
    $other++;
  }
}

prepare_country();
read_data();

# 1.
for my $entry (@attendee_list) {
  $total_cnt++;
  apply_filter(\&filter_SOSP, $entry); # `$entry' is a reference
  $sosp_cnt++;
}

# 2.
for my $entry (@attendee_list) {
  apply_filter(\&filter_workshop, $entry);
}

# 3.
for my $entry (@attendee_list) {
  apply_filter(\&filter_SOSP, $entry);
  apply_filter(\&filter_regtype, $entry);
}

for my $entry (@attendee_list) {
  apply_filter(\&filter_SOSP, $entry);
  apply_filter(\&filter_geo, $entry);
}

for my $entry (@attendee_list) {
  apply_filter(\&filter_SOSP, $entry);
  apply_filter(\&filter_company, $entry);
}

print "total_cnt\t", $total_cnt, "\n";
print "sosp_cnt\t", $sosp_cnt, "\n";
# print Dumper \%workshop_cnt;
dump_hash(\%workshop_cnt);

print "\nBreak down of regtype:\n";
print "Students\t", $regtype_student, "\n";
print "Non-Students\t", $regtype_nonstudent, "\n";
# print Dumper \%regtype_cnt, "\n";
dump_hash(\%regtype_cnt);

print "\nBreak down of geographic locations:\n";
# print "Students: ", $regtype_student, "\n";
# print "Non-Students: ", $regtype_nonstudent, "\n";
# print Dumper \%geo_cnt, "\n";
dump_hash(\%geo_cnt);
print "\n";
dump_hash(\%continent_cnt);

print "\nBreakdown of companies:\n";
print "student\t$student_academics\n";
print "nonstudent\t$nonstudent_academics\n";
print "industry\t$industry\n";
print "other\t$other\n";

# print Dumper \%company_list;


# for $entry in 
# print Dumper \@attendee_list;

close DATA;
