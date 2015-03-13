#!/usr/bin/perl

my $flag = "";

#use executable (or link) in current dir, if possible
#anyone not having compass installed can just make an executable
# empty script...

if ( -x compass) {
    $compass = "compass";
} else { 
    # owenism
    $compass = "/usr/local/src/BloBB_CompaSS_050315/bin/compass";
}

$infn=$ARGV[0].".soft";
$outfn=$ARGV[0].".compass";
$plfn=$ARGV[0].".pl";

# read the edge weights.
open(PL,$plfn) or die "$plfn could not open";
$_=<PL>; chomp;
($n,$junk) = split;
#skip the vertices
for ($i=0; $i<$n; ++$i){
    $_ = <PL>; chomp;
    @junk = split;
    # $junk[4] is number of shapes
    for ($j=0; $j < $junk[4]; $j++){ $_ = <PL>;}
}
# get num edges
$_=<PL>; chomp; $num_edges = $_;

print "This graph has $num_edges edges\n";

for ($i=0; $i < $num_edges; $i++) {
    $_=<PL>; chomp; @info = split;
    $wgt{"$info[0];$info[1]"} = $info[2];
}


$w=$ARGV[1];

while (1) {
    
    open(COMP,"$compass $infn $outfn --soft $flag | ") or die "cannot run compaSS";
    
    
    while (<COMP>) {
        # print;
        chomp;
        if (/CompaSS[']s runtime: ([\d.]+)s.*/) {
           $compass_time = $1;
        }
    }  
    close COMP;
      
    open(RES,"$outfn");
    $_ = <RES>; chomp; $res_w = $_;
    $_ = <RES>; chomp; $res_h = $_;

    die "no results obtained" unless defined $res_w and defined $res_h;

    if ($res_w > $w) {
        if ($flag eq "") {
        # set aspect ratio bound to prevent too much width
          $flag = "--HIER_OUTLINE_AR ".($w*$w/($res_w*$res_h*1.05));
        }
        else {
           # the maximum of 1 may kill us??
           print STDOUT "ERROR, On retry with aspect ratio flag $flag I still failed\n";
           exit 1;
        }
   }
   else {
        print "COMPASS TIME $compass_time\n";
        print "compaSS dimensions are width $res_w and height $res_h, area ",($res_w*$res_h),"\n";
        $wgtd_wirelen = compute_wirelen();
        print "compaSS wirelen is $wgtd_wirelen\n";
        exit 0; 
  }
}

sub compute_wirelen {
    # get placement data from RES.  We're about to start its third line
    # compass doc says a a blank line terminatates

    # ARRRGH compaSS does NOT preserve module order.  We'll have to try to infer
    # their relationships from the sizes!!!!  And this will only work with hard sizes
    # (but since we want to run this code anyway, we will just warn when it fails)
    open(SOFT,$infn) or die "opening compass input";
    $_=<SOFT>; # skip n, which we know
    for ($i=0; $i<$n;++$i) {
        $_ = <SOFT>; chomp; ($area1,$aspect1,$junk) = split;
        $mod_width = sqrt($area1/$aspect1);
        $mod_height = $area1/$mod_width;
        $mw[$i] = $mod_width; $mh[$i]=$mod_height;
        # print "module $i is $mod_width by $mod_height\n";
    }
    close SOFT;

    my $epsilon = 0.01;

    my @oldnum = ();   
    my @taken;


    $junk=<RES>; #skip n
    my $mctr=0;    
    while (<RES>) {
        chomp;
        last if /^\s*$/;
        ($ww,$hh) = split;
    
        #print "lookfor $ww, $hh\n";
        # see which module might be this one
        for ($j=0; $j<$n;$j++) {
            if (abs($ww-$mw[$j]) + abs($hh-$mh[$j]) < $epsilon and not $taken[$j]) {
                $taken[$j] = 1;
                $oldnum[$mctr] = $j;
                # print "$mctr used to be $j\n";
                last;
            }
        }
        print "WARNING, cannot remap modules after compass, wirelengths are junk\n" unless defined $oldnum[$mctr];
        $mctr++;

    }
    print "blank line is $_ \n";
    # global $n still says how many modules. Read placed origins
    for ($i=0; $i < $n; $i++) {
        $_ = <RES>; chomp;
        ($x,$y) = split;
        # note, we remap so everything in terms of original vtx nums
        $x[$oldnum[$i]] = $x; $y[$oldnum[$i]] = $y;
        # print "module $oldnum[$i] is at $x, $y\n";
    }

    $tot = 0;
    $ectr=0;
    foreach $edge (keys %wgt) {
        $w = $wgt{$edge};
        ($v1,$v2) = split /;/ , $edge;
        $dist = sqrt( ($x[$v1]-$x[$v2])**2 +  ($y[$v1]-$y[$v2])**2);
        # print "edge $ectr ($v1,$v2) dist $dist weight $w\n";
        $tot += $w * $dist;
        $ectr++;
    }
    return 2*$tot;  #2 times since we're supposed to process ordered pairs of vertices
}
