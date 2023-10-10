#!/bin/bash
#
# Elliot Forcier-Poirier
# elliot.forcier-poirier@mail.mcgill.ca
# McGill University
#

#Assure we have the right number of arguments.
if [ $# != 1 ]
then
        echo "Usage ./logparser.bash <logdir>"
        exit 1
fi

#Assure the argument is a directory, if not return an error and pass it to standard error, not standard output.
if ! [ -r $1 ]
then
        echo "Error: $1 is not a valid directory name"
        exit 2
fi

#Make sure we are creating new csvs
rm senderdata.csv
rm receiptdata.csv
rm deliveryData.csv

for log in $( find $1 )
do
	#Generate the initial Broadcaster Process, and all the in`formation initially available. 
	awk  '{ if ($6 == "DISLXXX.gcl.GCL$GCLOutBoxDespatcher" && $10 == "Sending" && $14 ~ "teach-node" )  {print substr($9,1,19) "," substr(substr($14,40,length($14)-39),0,length(substr($14,40,length($14)-39))-1) "," substr($14, 16, 19) "," $4 }}' $log >> senderdata.csv
	
	#Generate the Initial file with only the receipt data.
	awk -v var="${log:51:5}"  '{ if ($9 == "Received" && $13 ~ teach-node ) {print substr($10,2,4) "," $4 "," $13 "," var}}' $log >> receiptdata.csv
	
	#Generate the Initial file with only the Delivered data.
	awk -v var="${log:51:5}" '{ if ($6 == "DISLXXX.gcl.GCL$GCLDeliveryThread" && $9 == "Delivering" ) {print substr(substr($12,40,5),1,length(substr($12,40,5))-1) "," $4 "," substr($12,16,19) "," var}}' $log >> deliverydata.csv
done

#Assure the csv doesn't overlap with previous data
rm logdata.csv

#Combine the three files into logdata.csv
while IFS="," read -r col1 col2 col3 col4 
do
	receivingNode="$1/${col3/:/.}.log"
	echo $senderNode
	echo $col1 
	echo $col2
	echo "${col3:14:5}"
	col5=$(awk -v var1="$col1" -v var2="$col2" -v var3="${col3:14:5}" 'BEGIN {FS=","} ; { if ( $1 == var2 && $3 == var1 && $4 == var3 ) {print $2} }' receiptdata.csv)
	col6=$(awk -v var1="$col1" -v var2="$col2" -v var3="${col3:14:5}" '{ if ( $1 == var2 && $3 == var1 && $4 == var3 ) {print $2} }' deliverydata.csv) 
	echo $col5
	echo $col6
	echo "$col1, $col2, $col3, $col4, $col5, $col6" >> logdata.csv
done < senderdata.csv
