# Tag Cloud Drawing code by Kaser and Lemire.



This is the code used for the following paper :

Owen Kaser and Daniel Lemire, Tag-Cloud Drawing: Algorithms for Cloud Visualization. In proceedings of Tagging and Metadata for Social Information Organization (WWW 2007), 2007. http://arxiv.org/abs/cs.DS/0703109
Have a look at our slides:

http://www.slideshare.net/lemire/tagcloud-drawing-algorithms-for-cloud-visualization-presentation



At the same level of this README-TAGPLACMENT, there are
directories "floorplan-code" and "flat-tables".

Most items are in floorplan-code.  It has a README and
subdirectories for src, data and bin.

In src, the Makefile can also do some things involving flat-tables.

In floorplan-code/src, try
     make
     make placetest
     make guttest
     make classictest

The latter two require that your setup have Firefox and other things similar
to ours...  The first two might work okay on any platform,

# INSTALL

Use: (from the src subdirectory)
     
      make
      cd ../data
      ../bin/floorp owen
      pdflatex owen
      xpdf owen.pdf
     
The first parts of this can be achieved by "make placetest"


The input file would be called 'owen.pl' and for debugging, it outputs
TeX.  We also output html, so you can fire up a browser and look at owen.html.

Also, there are  scripts that superficially extract "keywords" and their collocation information, determine bounding box sizes,   floorplan and then show the html
    
The top-level script for this is "gut.sh", eg

         gut.sh <path to some text file> <number of tags to take>

There is a make target to do this, too...it will process some source code file to get its tags!

The scripts are definitely quick-prototype quality and if they actually work for you....wow!

File "owen1.pl" is a more complex sample input


This _was_ a CAD tool...which means it does more than we need.  For instance,
it reserved "wiring spaces" between boxes (depending on how many connections
are specified between boxes).  Also, each initial box has a specified "upward"
and "righward" pull  (certain chip modules may have strong reasons to want
to be on the upper-left area of the chip, for instance).  The code is also
capable of choosing between different alternative shapes for each box.