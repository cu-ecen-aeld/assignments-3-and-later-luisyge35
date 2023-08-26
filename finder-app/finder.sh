#!/bin/sh

if [ $# -ne 2 ]; then
  echo "You must provide two parameters [path, textString]"
  exit 1
fi

filesDir=$1
searchStr=$2

if ! [ -e "$filesDir" ]; then
  echo "Invalid path"
  exit 1
fi

cd $filesDir
files=$(find . -type f)
fileNum=$(echo $files | wc -w)
numMatches=$(echo $files | grep -r $searchStr | wc -w)

echo "The number of files are $fileNum and the number of matching lines are $numMatches"