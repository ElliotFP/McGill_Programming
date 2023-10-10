#!/bin/bash
#
#Elliot Forcier-Poirier
#elliot.forcier-poirier@mail.mcgill.ca
#Faculty of Arts
#

#Code 1 Error : More than 2 Arguments
if [ $# -ne 2 ] 
then
	echo "Usage: namefix.bash <inputfile> <outputfile>" >&2
	exit 1
fi

#Make output file if no exist
if ! [ -e $2 ]
then
	touch $2
fi

#Code 2 Error : Same Input and Output
if [ "$(basename "$1")" == "$(basename "$2")" ]
then 
	echo "Usage: namefix.bash <inputfile> <outputfile>" >&2
	echo "Error: input  $1 and output $2 are the same"
	exit 2
fi 

#Code 3 Error : Input File not readable
if ! [ -r "$1" ] 
then 
     	echo "Usage: namefix.bash <inputfile> <outputfile>" >&2
	echo "Error: input is not readable"
	exit 3
fi

#Code 4 Error : Output File not writable
if ! [ -w $2 ]
then
	echo "Usage: namefix.bash <inputfile> <outputfile>" >&2
	echo "Error: Output is not readable"
	exit 4
fi

#Output file is a directory
if [ -d "$2" ]
then
	outputPath="$2/$(basename $1)"
	if [ -d "$outputPath" ]
	then
		echo "Usage: namefix.bash <inputfile> <outputfile>" >&2
		echo "$outputPath is a directory"
		exit 4
	fi
	/home/2013/jdsilv2/206/mini2/namefix $1 $outputPath
	cat $outputPath
else
	/home/2013/jdsilv2/206/mini2/namefix $1 $2
	cat $2
fi
