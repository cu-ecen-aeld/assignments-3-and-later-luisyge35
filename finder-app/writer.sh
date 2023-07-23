#!/bin/bash

if [ $# -ne 2 ]; then
  echo "You must provide two parameters [fullPath, textString]"
  exit 1
fi

writeFile=$1
writeStr=$2

writeDir=$(dirname "$writeFile")
mkdir -p $writeDir

echo "$writeStr" > "$writeFile"