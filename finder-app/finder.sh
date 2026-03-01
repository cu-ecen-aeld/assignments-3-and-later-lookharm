#!/bin/bash

if [ $# -ne 2 ]
then
	echo 'Please provide filesdir and searchstr. e.g finder.sh <filesdir> <searchstr>'
	exit 1
fi

filesdir=$1
searchstr=$2

if [ ! -d $filesdir ]
then
	echo "No directory $filesdir"
	exit 1
fi

grep -rc $searchstr $filesdir | awk -F':' '{if ($2 > 0) { count++; sum += $2 }} END { print "The number of files are " count " and the number of matching lines are " sum }'



