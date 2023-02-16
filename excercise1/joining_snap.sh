#!/bin/bash

#This simple script perform the joining of the partial snap files 
#produced by a run of the game of life program

l=$(ls snap | grep  snap_static_*_00* | cut -d'_' -f 3 | sort -u)
for i in ${l}
do
	echo $(ls snap | grep snap_static_${i})
	cat snap/snap_static_${i}* > snap/span_static_${i}.pgm
	rm snap/snap_static_${i}* 	
	echo ${i}
done

l=$(ls snap | grep  snap_ord_*_00* | cut -d'_' -f 3 | sort -u)
for i in ${l}
do
	echo $(ls snap | grep snap_ord_${i})
	cat snap/snap_ord_${i}* > snap/span_ord_${i}.pgm
	rm snap/snap/snap_ord_${i}*	
	echo ${i}
done
