#!/bin/bash

#Perform the joining of the output file produced by the game of life program

var=$(ls output | grep static_00 | wc -l)
if [ ${var} -gt 0 ]
then
	cat output/out_static_00* > output/out_static.pgm
	rm output/out_static_00*
fi

var=$(ls output | grep ord_00 | wc -l)
if [ ${var} -gt 0 ]
then
	cat output/out_ord_00* > output/out_ord.pgm
	rm output/out_ord_00*
fi


