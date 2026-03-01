#!/bin/bash

if [ $# -ne 2 ]
then
	echo 'Please provide writefile and writestr. e.g writer.sh <writefile> <writestr>'
	exit 1
fi

writefile=$1
writestr=$2

dirpath=$(dirname $writefile)

mkdir -p $dirpath
touch $writefile
echo $writestr > $writefile
