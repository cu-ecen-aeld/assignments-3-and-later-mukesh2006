#!/bin/sh

## added the bin/sh to overcome the error ./finder-test.sh: line 61: /home/finder.sh: not found

filesdir=$1
searchstr=$2

if [ "$#" -ne 2 ]; then
 echo "Usage: ./find.sh filesdir searchdir"
 exit 1;
fi

if [ ! -d "$filesdir" ]; then
  echo "The direcotry " $filesdir " does not exist"
  exit 1;
fi

X=$(find "$filesdir" -type f | wc -l)
Y=$(grep -r "$searchstr" "$filesdir" | wc -l)

echo "The number of files are " $X " and the number of matching lines are " $Y
