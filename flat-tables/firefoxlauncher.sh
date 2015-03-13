os=`uname`
echo $os
if [ $os = "Darwin" ]; then 
  echo "MacOS detected";
  open -a Firefox $1
else
  echo "I assume Linux";
  firefox -remote "openurl($1)" 
fi