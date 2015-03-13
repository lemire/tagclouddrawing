#!/bin/bash

# ensure no firefoxes running
if ps aux | grep -q  '[f]irefox' ; then
  echo kill all firefoxes and retry
  exit 1
fi


echo running experiments at `date`


for k in 20 50 100 200 ; do

  echo "Setting k=$k"
  firefox &
  sleep 5

  #owenism follows
  datadir="/home/owen/lemur/litOLAP/data"
  for f in aaard10 b033w10 callw10 dblnr11 ebacn10 fb10w11 g138v10 h8ahc10\
           icfsh10 jandc10 kalec10 lacob11 magoz10 nativ10 oldno10 pam1w10\
           rbddh10 samur10 tagod10 ulyss12 ; do

      fullnm=$datadir/$f.txt
      echo Processing $fullnm
      #wc -l $fullnm
      gut.sh $fullnm $k
      cp collocates.input "../../flat-tables/gutenberg-inputs/$f-$k.input"
      pl2compass.pl temporary.pl --soft > temporary.soft
      
      # on size >300 or so, it may crash or exceed the 550 width :(
      runcompass.pl temporary 550
      pl2compass.pl temporary.pl --hard > temporary.soft
      runcompass.pl temporary 550
      pl2compass.pl temporary.pl > temporary.soft
      runcompass.pl temporary 550
  done
  
  jobs
  kill -9 % 
  sleep 2
  foxpid=`ps aux | grep "[/]usr/lib/mozilla-firefox/firefox-bin" | sed 's/\s\s*/ /g' | cut -f 2 -d ' '`
  echo foxpid is $foxpid
  kill $foxpid

done