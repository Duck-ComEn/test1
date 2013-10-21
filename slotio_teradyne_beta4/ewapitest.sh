#!/bin/bash

startSlot=56
endSlot=69

# Turn off power to multiple slots
utilDrivePower()
{
	if [ $1 = 1 ]; then
		voltValue=3000
	else
		voltValue=0
	fi
	
	for((i=$startSlot; i<$endSlot+1; ++i));do
		./wapitest stv $i $voltValue 0
	done
}


# Read slot numbers
promptSlotNumber()
{
	echo -n "Enter starting slot number -> "
	read  startSlot

	echo -n "Enter ending slot number   -> "
	read  endSlot
}

# Iniatalize
initTest()
{
	# Turn off power
    utilDrivePower 0
	
    # Remove all log files
	/bin/rm /var/tmp/slot*.log
}

# Iniatalize
finalizeTest()
{
	# List all instances of wapitest
	ps -a | grep wapitest
	
	# List all log files
	ls /var/tmp/
}



# Echo/Read/Write on single slot multiple times
extendedTest01()
{
	echo -n "Enter slot number -> "
	read  testSlot

	for((i=0; i<10; ++i));do 
		./wapitest t01 $testSlot -deb7 -q -L1; 
	done
}
 

# Echo/Read/Write on multiple slots
extendedTest02()
{
	promptSlotNumber
	initTest
	
	for((i=$startSlot; i<$endSlot+1; ++i));do
		./wapitest t01 $i -deb7 -q -L1 &
		sleep 2
	done
	
	finalizeTest
}
 
# 1000 Echo multiple on slots
extendedTest03()
{
	promptSlotNumber
	initTest
	
	for((i=$startSlot; i<$endSlot+1; ++i));do
		./wapitest t08 $i -deb7 -L1 -q &
		sleep 20
	done
	
	finalizeTest
}

# Read on multiple on slots
extendedTest04()
{
	promptSlotNumber
	initTest
	
	for((i=$startSlot; i<$endSlot+1; ++i));do
		./wapitest t07 $i -deb7 -L1 -q &
	done
	
	finalizeTest
}


# Single Read on multiple slots
extendedTest05()
{
	promptSlotNumber
	
	echo -n "Number of bytes to read    -> "
	read  size

	initTest

	for((i=$startSlot; i<$endSlot+1; ++i));do
		./wapitest rdm $i 0x08331000 $size -deb7 -q &
	done
	
	finalizeTest
}

# Simulate short test on multiple slots
extendedTest06()
{
	promptSlotNumber
	initTest
	
	for((i=$startSlot; i<$endSlot+1; ++i));do
		./wapitest t09 $i -deb7 -L3 -q &
	done
	
	finalizeTest
}

# DIO Firmware update
extendedTest07()
{
	promptSlotNumber
	
	for((i=$startSlot; i<$endSlot+1; ++i));do
		./wapitest gcv $i &
	done
	
}

extendedTest99()
{
#	echo -n "Enter file name -> "
#	read  fileName
#	echo $filename

	echo -n "Enter slot number -> "
	read  slotNo

    filename=/\var/\tmp/\slot$slotNo\*.log

    echo  'set xdata time' > temp.plt
    echo  'set timefmt "%Y/%m/%d,%H:%M:%S"' >> temp.plt
    echo  'plot "-" using 1:2 with lines' >> temp.plt
	
#	2011/03/11,17:48:13,Exit : getCurrentTemperature, rc=0, 3500

    grep 'getCurrentTemperature,' $filename | sed 's/,Exit : getCurrentTemperature, rc=.,//g' >> temp.plt
 	
	 
	echo  'end' >> temp.plt
	echo  'pause -1' >> temp.plt
	gnuplot "temp.plt"

}



echo "1) Echo/Read/Write on single slot multiple times"
echo "2) Echo/Read/Write on multiple slots"
echo "3) 1000 Echo on multiple slots"
echo "4) Multiple Reads on multiple slots"
echo "5) Single Read on multiple slots"
echo "6) Simulate short test on multiple slots"
echo "7) DIO Firmware update"
echo "x) Exit"
echo -n "Which test would you like to run? "
read case;

case $case in
    1) extendedTest01;;
    2) extendedTest02;;
    3) extendedTest03;;
    4) extendedTest04;;
    5) extendedTest05;;
    6) extendedTest06;;
    7) extendedTest07;;
	99) extendedTest99;;
    x) exit
esac 
