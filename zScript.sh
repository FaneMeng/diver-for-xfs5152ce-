#!/bin/bash

workdir=$(cd $(dirname $0); pwd)
project_name="${workdir##*/}"

if [ $# -eq "1" ]
then
	if [ "$1" = "env" ]
	then
		export ARCH=arm
		export CROSS_COMPILE=arm-linux-gnueabihf-
		echo "Compiling environment is set."
	elif [ "$1" = "mod" ]
	then
		cd ..
		cp -r ~/share/$project_name/ ./
	        cd $workdir
	        make
	elif [ "$1" = "app" ]	
	then
		cd ..
                cp -r ~/share/$project_name/ ./
                cd $workdir
		arm-linux-gnueabihf-gcc app.c -o app -static
	elif [ "$1" = "nfs" ]
	then
		cp *.ko app ~/nfs
	elif [ "$1" = "help" ]
	then
		cat help.txt
	else
		echo "You provided an unvalid parameter."
		echo "Try \"bash `basename $0` help\""
	fi
else 
	echo "You provided $# parameters,but 1 are required."
	echo "Usage: bash `basename $0` parameter"
fi
