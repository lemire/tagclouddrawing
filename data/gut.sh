#!/bin/bash

#use: feed it a text file, with or (pref) without its header/trailer
# text file is probably from Project Gutenberg, but source code etc would also do...
#
# (file name is $1 and number of tags is $2)

# it will create a phony floorplan data set, floorplan with it,
# create a html file and then view it.

touch temporary.html temporary.pl
rm temporary.html temporary.pl

./collocation-tags.pl $1 $2 &&\
#floorplan with it
  ../bin/floorp temporary
#now, got temporary.html


if ( ! ./firefoxlauncher.sh "file://$PWD/temporary.html" 2>/dev/null); then
  if ( ! ./firefoxlauncher.sh  "file://$PWD/temporary.html"); then
     echo "firefox is not being very nice.  Maybe you have an open "
     echo "downloads window that you need to close, seems to help"
  fi
fi
