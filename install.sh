#!/bin/bash

if [ $EUID -ne 0 ]
then
	echo "install procedure requires root permissions"
	exit 1
fi

cp medioed /usr/bin
if [ $? -ne 0 ]
then
	echo "please build program before installing"
	exit 1
fi
