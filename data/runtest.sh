#!/bin/bash

#use: feed it a file whose first column can be a size specifier
#(1-9) and whose remainder of the line is a "cloud" entry

# it will create a phony floorplan data set, floorplan with it,
# create a html file and then view it.

# an example input file is example.input

# These scripts dedicated to Rube Goldberg

FLATTBLS=../../flat-tables

#create a *.csv file
cp $1 $FLATTBLS/horridhack.input
( pushd $FLATTBLS ; converttag.sh $PWD/horridhack.input )
sleep 5
cp $FLATTBLS/horridhack.input.csv .

touch temporary.html temporary.pl
rm temporary.html temporary.pl

#create temporary.pl
make_datafile $1 horridhack.input.csv
#floorplan with it
../bin/floorp temporary
#now, got temporary.html

#on my machine, mz brings up mozilla with a file url...
mz temporary.html