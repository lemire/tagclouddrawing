#!/usr/bin/perl

# below worked except sed did not handle \240 properly

#egrep '^[^,#]*[a-zA-Z0-9][^,]*,[^,]*,[^,]*,[^,]*$' $1 |\
#  sed 's/^\([^,]*\),[ ]*\([^,]*\).*/\2\1/' >$2

open(OUT,">$ARGV[1]") || die "opening out";
open(IN,$ARGV[0]) || die "opening in";

while (<IN>) {
    if (/^[^,#]*[a-zA-Z0-9][^,]*,[^,]*,[^,]*,[^,]*$/) {
           s/^([^,]*),[ ]*([^,]*).*/\2\1/;
           print OUT;
       }
        last if /^\s+,.*/;  #sentinal seen
}
  close OUT;



