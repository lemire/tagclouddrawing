#!/bin/bash

# programming gods are concocting a BIG lightening bolt for this

yourfilename=$1
#yourfilename="/home/owen/lemur-svn/flat-tables/bar.input"   #example
#backslashify the path separators (only)
fnname=`echo $yourfilename | sed 's/[/]/\\\\\\//g'`

sed "s/XYZZY/$fnname/" alsoopeninfirefox.template > alsoopeninfirefox.html

if ( ! ./firefoxlauncher.sh "file://$PWD/alsoopeninfirefox.html" 2>/dev/null); then
    echo "no firefox, so we make one"
    firefox &
    sleep 10
    if ( ! ./firefoxlauncher.sh "file://$PWD/alsoopeninfirefox.html" 2>/dev/null); then
        echo "this REALLY should not happen"
    fi
fi
