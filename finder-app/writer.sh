#!/bin/bash
writefile=$1
writestr=$2
if [ $# -eq 2 ]
then
	echo ${writestr} > ${writefile}
	if [ ! -f ${writefile} ]
	then
		echo "File not created properly"
		exit 1 
	fi	
else
	echo "The number of arguments required are 2"
	exit 1
fi 
