#!/bin/bash

./analyze.pl gutenbergrun.log
cp *tbl.tex ../../TagClouds/latex

echo "done, tables moved to the paper dir"
