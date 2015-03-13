#!/usr/bin/perl

use Cwd;
# scan a text file looking for the K, eg 100, most frequent 6+ letter words
# 
# then build collocation (in same line) info over these words
# 
# then output an "*.input" file, tag importance quantitized linearly
# into 10 buckets.  
# convert to a .cvs file via DL's javascript magic
# finally, build a .pl file
#
# author: OFK, 29 Jan 2007

$fn = $ARGV[0];  #filename

$k = $ARGV[1];  #num items to take


open(INF,$fn) or die "cannae open your $fn\n";


# Some form of header stripping, stemming, etc  would probably
# be very useful.  But, the purpose is primarily to get something
# to lay out.  The usefulness of the data being laid out is not
# so important for this project....

# note: uses fixed-name files and does not worry about validity checking
#  (In other words, this is crufty.)


while (<INF>) {
    chomp;
    next if /^\s*$/;
    tr /A-Z/a-z/;  # lowercase

    foreach $word (split) {
       $word =~ s/[^a-z]//g;  # alpha only

        next if length $word < 6;  #no short words
        $count{$word}++;
    }
}

close INF;

# get most common k words
@frequent_words = sort {$count{$b} <=> $count{$a}} (keys %count);
$#frequent_words = $k-1; #truncate; aargh, had one too many!

$mx = $count{$frequent_words[0]};
$mn = $count{$frequent_words[$k-1]};
$r = $mx - $mn;
$b = $r/10;  #bucketsize

open(OWENS,">collocates.input") || die "creating collocates file";

$wctr = 0;
foreach $w (@frequent_words) {
    #print "$w : $count{$w}\n";
    $bkt = int(10*($count{$w} - $mn) / ($r+1));
    print OWENS "$bkt$w\n";
    $serial{$w} = $wctr++;
}
close OWENS;

$FLATTBLS="../../flat-tables";  # make assumptions about directory layout...

#create a *.csv file
$dir = cwd();
system( "cd $FLATTBLS; ./converttag.sh $dir/collocates.input" );
sleep 5;

#oh, horrors...

system ("./make_datafile collocates.input collocates.input.csv --noedges");

#leaves edges undone, which is okay: we'll compute collocates now

open(INF,$fn) or die "cannae open your $fn the second time, weird\n";

while (<INF>) {
    chomp;
    next if /^\s*$/;
    tr /A-Z/a-z/;  # lowercase

    $prev_id = undef;
    foreach $word (split) {
       $word =~ s/[^a-z]//g;  # alpha only

       $id = $serial{$word};
       next unless defined $id;

       # for some time, this incorrectly processed as a directed graph....
       #note test against selfloops, which input file handles by other means
       $collocate{"$prev_id $id"}++ if defined $prev_id and $prev_id > $id; 
       $collocate{"$id $prev_id"}++ if defined $prev_id and $prev_id < $id; 
       $prev_id = $id;
    }
}
close INF;

open(OUTF,">>temporary.pl") or die "cannot append edges to file";

@edges = keys %collocate;
#print number of edges
@good_edges = grep {$collocate{$_} > 1} @edges;

print OUTF scalar(@good_edges),"\n";

foreach $ed (@good_edges) {
  print OUTF "$ed ",$collocate{$ed},"\n";
}

print OUTF "0\n" ;  #no hyperedges
close OUTF;

#and we are done
