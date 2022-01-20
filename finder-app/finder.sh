#!/bin/bash
filesdir=$1
searchstr=$2

if [ $# -eq 2 ]
then


	if [ -d ${filesdir} ]
	then
		count=$(ls ${filesdir}| wc -l)
		Y=$(grep -r ${searchstr} ${filesdir}| wc -l)
		echo "The number of files are ${count} and the number of matching lines are ${Y}"
	else
		echo "filesdir does not represent a directory"
		exit 1
	fi
else
	echo "The expected number of arguments are 2"
	exit 1
fi 
