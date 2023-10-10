#!/bin/bash
#
# Elliot Forcier-Poirier
# elliot.forcier-poirier@mail.mcgill.ca
# Faculty of Arts
#

args=( "$@" )

#Checking that it is the right amount of arguments
if ! [ $# == 2 ] && ! [ $# == 3 ] 
then 
	echo "Usage: ./primechk.bash -f <numbersfile> [-l]"
        exit 1
fi

#Placements for the file
if [ $1 == "-f" ]
then
	file=$2
else 
	if [ $2 == "-f" ] && [ $1 == "-l" ]
	then
       		file=$3
	else
        	echo "Usage: ./primechk.bash -f <numbersfile> [-l]"
        	exit 1
	fi
fi
	
#Checking if the file exists
if ! [ -e $file ]
then 
	echo "Usage: ./primechk.bash -f <numbersfile> [-l]"
	echo "Error: $file does not exist"
	exit 2
fi

greatestPrime=0

#Separate in two cases depending on if the function has the -l flag.
if [ "$1" = "-l" ] || [ "$3" = "-l" ]
then
	#For loop to check if the numbers are prime uses sed to take only the integers
	for nb in $(sed -nE '/^[0-9]+$/p' < $file)
	do 	
		if [ "$(/home/2013/jdsilv2/206/mini2/primechk $nb)" == "The number is a prime number." ] && [ $greatestPrime -lt $nb ]
                then
                        greatestPrime=$nb
                fi
	done
	
	if ! [ $greatestPrime == 0 ]
        then
                echo $greatestPrime
      
        else
                echo "Usage: ./primechk.bash -f <numbersfile> [-l]"
                echo "Error: Did not find any prime numbers in $file"
                exit 3
	fi
else
	#For loop to check if the numbers are prime uses sed to take only the integers
	for nb in $(sed -nE '/^[0-9]+$/p' < $file)
	do
		if [ "$(/home/2013/jdsilv2/206/mini2/primechk $nb)" == "The number is a prime number." ]
        	then
			echo $nb
		fi
	done
fi

