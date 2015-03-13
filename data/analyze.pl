#!/usr/bin/perl

while (<>) {

    chomp;
# sync
    next unless /Setting.*/ or /Processing.*/;

    if (/Setting k=(\d+)/) {
        $k=$1;
        $kvals{$k}=1;

    }
    elsif (/Processing (.*)/) {
        $f = $1;
        $fvals{$f}=1;

        while (<>) {
            chomp;
            if (/Elapsed time to completion of rec_placement is (\d+) sec (\d+) microsec/) {
                $rp{"$f;$k"} = $1 * 1000 + $2/1000;  #multiple iterations overwrite, good
            }
            elsif (/Elapsed time to completion of size is (\d+) sec (\d+) microsec/) {
                $size{"$f;$k"} = ($1 * 1000 + $2/1000) - $rp{"$f;$k"};
                ++$iters{"$f;$k"};
            }
            elsif (/Elapsed time to completion of top_down_size is (\d+) sec (\d+) microsec/) {
                $tds{"$f;$k"} = 1000 * $1 + $2 / 1000;
            }
            elsif (/Euclidean sum distance of this solution is ([\d.e+-]+)/) {
                $mcd{"$f;$k"} = $1/1000;
            }
            elsif (/The area is about ([\d.e+-]+)/) {
                $mca{"$f;$k"} = $1/1000;
            }
            elsif (/Euclidean sum distance of ordered greedy solution is ([\d.e+-]+)/) {
                $ogd{"$f;$k"} = $1/1000;
            }
            elsif (/The \(ordered\) greedy area is about ([\d.e+-]+)/){
                $oga{"$f;$k"} = $1/1000;
            }
            elsif (/Euclidean sum distance of scrambled greedy solution is ([\d.e+-]+)/) {
                $sgd{"$f;$k"} = $1/1000;
            }
            elsif (/The scrambled greedy area is about ([\d.e+-]+)/){
                $sga{"$f;$k"} = $1/1000;
                $compass_ctr=0;
            }
            elsif (/COMPASS TIME ([\d.]+)/) {
                $ctime{"$f;$k;$compass_ctr"} = $1*1000;
                $cht{"$f;$k"} = $1*1000 if $compass_ctr == 1;
                $cst{"$f;$k"} = $1*1000 if $compass_ctr == 0;
            }
            elsif (/compaSS dimensions are width [\d.]+ and height [\d.]+, area ([\d.]+)/){
                $ca{"$f;$k;$compass_ctr"} = $1/1000;
                $csa{"$f;$k"} = $1/1000 if $compass_ctr == 0;
                if ($compass_ctr == 1) {
                    $cha{"$f;$k"} = $1/1000;  #compass hard option is the only that does not have area fudging
                    $cha_vs_csa{"$f;$k"} = 
                        ($cha{"$f;$k"} - $csa{"$f;$k"})/$cha{"$f;$k"};  #"soft smaller than hard"
                }
            }
            elsif (/compaSS wirelen is ([\d.]+)/){
                if ($compass_ctr == 1) {
                    $chd{"$f;$k"} = $1/1000;
                }
                ++$compass_ctr;
                last if $compass_ctr == 3;
            }
        }
    }
}

@sortedKs = sort {$a<=>$b} (keys %kvals);

open(OUT,">speed_tbl.tex") or die "opening tbl1";
print OUT "\\begin{tabular}{r|rrrrr}\\hline\n";
print OUT "No. Tags & Iters & Finish & Size & C-hard & C-soft\\\\\\hline\n";
foreach $k (@sortedKs) {
    printf OUT "%d & %5.1f & %5.1f & %5.1f & %6.1f & %6.1f \\\\\n",
          $k,
         avgByK($k,10,%iters),
         avgByK($k,10,%tds),
         avgByK($k,10,%size), 
#        avgByK($k,10,%rp), 
         avgByK($k,10,%cht),
         avgByK($k,10,%cst);
}
print OUT "\\hline\n\\end{tabular}\n";
close OUT;


open(OUT,">area_tbl.tex") or die "opening tbl2";
print OUT "\\begin{tabular}{l|rrrr}\\hline\n";
print OUT " & & \\multicolumn{2}{c}{Greedy}& \\\\\n";
print OUT "No. Tags & Min-cut & (sorted) & (random) & \\textsc{compaSS} \\\\\\hline\n";
foreach $k (@sortedKs) {
    print OUT "$k ", bfmin(
        " & ", avgByK($k,1,%mca),
        " & ", avgByK($k,1,%oga), 
        " & ", avgByK($k,1,%sga),
        " & ", avgByK($k,1,%cha)),"\\\\\n";
}
print OUT "\\hline\n\\end{tabular}\n";
close OUT;


open(OUT,">dist_tbl.tex") or die "opening tbl3";
print OUT "\\begin{tabular}{l|rrrr}\\hline\n";
print OUT " & & \\multicolumn{2}{c}{Greedy} &\\\\\n";
print OUT "No. Tags & Min-cut & (sorted) & (random) &\\textsc{compaSS}\\\\\\hline\n";
foreach $k (@sortedKs) {
    print OUT "$k ", bfmin(
        " & ", avgByK($k,1,%mcd), 
        " & ", avgByK($k,1,%ogd), 
        " & ", avgByK($k,1,%sgd),
        " & ", avgByK($k,1,%chd)),"\\\\\n";
}
print OUT "\\hline\n\\end{tabular}\n";
close OUT;


# just report on this, I guess
open(OUT,">softhard_tbl.tex") or die "opening tbl4";
print OUT "\\begin{tabular}{l|rrr}\\hline\n";
print OUT "Soft area& Hard Area & Soft smaller than hard \\\\\\hline\n";
foreach $k (@sortedKs) {
    print OUT "$k & ", avgByK($k,1,%csa),
        " & ", avgByK($k,1,%cha), 
        " & ", avgByK($k,1000,%cha_vs_csa),
"\\\\\n";
}
print OUT "\\hline\n\\end{tabular}\n";
close OUT;



# at odd numbered positions we have numbers to be compared
# but even numbered are separators to be passed verbatim

sub bfmin {
    my @ans = ();
    my $minval=1000000000;
    my $i;
    for ($i=0; $i < @_; $i++) {
        $minval = $_[$i] if ($i%2 == 1) and $minval > $_[$i];
    }

    # bold any that achieve min
    for ($i=0; $i < @_; $i++) {
        if ($i%2 == 0) {
            push @ans, $_[$i];
        } else {
            push @ans, "\\textbf{$_[$i]}" if $_[$i] == $minval;
            push @ans, $_[$i] if $_[$i] != $minval;
        }
    }
    return @ans;
}



sub avgByK {
    my ($k, $roundfactor, %arr) = @_;
    my ($sum,$count);
#    $sum=0; $count=0;
    
    foreach $ky (keys %arr) {
        ($junk,$kk) = split(/;/,$ky);
        next if ($kk != $k);
        
        $sum += $arr{$ky};
        $count++;
    }
    return int(0.5+$sum*$roundfactor/$count)/$roundfactor if $count != 0;

    print STDERR "Hey, no keys $k in array", %arr,"\n";
    return 0;
}
