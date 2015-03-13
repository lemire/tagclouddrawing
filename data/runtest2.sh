#!/bin/bash

#use: feed it one of  Daniel's csv tag sets

# it will create a phony floorplan data set, floorplan with it,
# create a html file and then view it.


touch temporary.html temporary.pl
rm temporary.html temporary.pl

#create temporary.pl

csv2owen.pl $1 horror.input
echo start making file
make_datafile horror.input $1 && \
  (echo start floorplanning ;
#floorplan with it
  ../bin/floorp temporary
#now, got temporary.html
   ) && \


#mz temporary.html
#on my machine, mz brings up mozilla with a file url...

if ( ! firefox -remote "openurl(file://$PWD/temporary.html)" 2>/dev/null); then
  if ( ! firefox file://$PWD/temporary.html); then
     echo "firefox is not being very nice.  Maybe you have an open "
     echo "downloads window that you need to close, seems to help"
  fi
fi
