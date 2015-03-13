#!/usr/bin/perl

# convert from .pl format to the "soft" format used by compass
# input from file, output to stdout

open(IN,"$ARGV[0]") or die "opening input file";

$hard = 0;  #hard means only use the centre shape
$soft = 0;  #soft means allow continuous deformation in range

for ($i=1; $i < @ARGV; ++$i) {
    $hard = 1 if $ARGV[$i] eq "--hard";
    $soft = 1 if $ARGV[$i] eq "--soft";
}

die "combo hard and soft does not work" if $hard and $soft;

$_ = <IN>;
chomp;
# get n

($n,$junk) = split / /;

print "$n\n";

for ($i=0; $i < $n; ++$i) {
    $_ = <IN>;
    chomp;
    
#    print "process $i\n";
    
    ($junk1, $area, $junk2, $junk3, $nshapes, $junk5, $junk6) =
        split / /;
    

    
    die "I need 3 shapes, you had $nshapes" unless $nshapes == 3;
    
    for ($sh=0; $sh < 3; ++$sh) {
        $_ = <IN>;
        chomp;
        ($x[$sh], $y[$sh]) = split;

        # ACCOUNT FOR DANIEL'S 2px SPACING by making every tag 2 units wider
        # neither the C code nor compass account for any required vertical padding
        # as long as we don't compare too much with measured screen values or with
        # daniel's line-breaking heights, it should be fair enough
        $x[$sh] += 2;
    }
    # now, adjust area for the 2 pixels too
    $area += 2*$y[1];
#    print STDERR "increase area by ",2*$y[1], " to $area\n";

    print " $area ";

    if ($hard) {
        print $area/($x[1] * $x[1])," ",$area/($x[1]*$x[1]),"\n";
    }
    elsif ($soft) {
        print $area/($x[2]*$x[2])," ",$area/($x[0]*$x[0]),"\n";
    }
    else {
        # have to fake the y dimension to achieve constant area :(
        print $area/($x[2]*$x[2])," ",
        $area/($x[2]*$x[2])," ",
        $area/($x[1]*$x[1])," ",
        $area/($x[1]*$x[1])," ",
        $area/($x[0]*$x[0])," ",
        $area/($x[0]*$x[0]),"\n";
    }
}


