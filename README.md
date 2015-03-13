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


Also, there are  scripts that superficially extract "keywords" and their collocation information, determine bounding box sizes,   floorplan and then show the html
