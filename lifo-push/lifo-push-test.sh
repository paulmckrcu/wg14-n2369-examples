#!/bin/bash
#
# Run a crude performance test of the various lifo-push implementations
for ((i=0;i<50;i++))
do
	for pgm in ./lifo-push ./lifo-push-london
	do
		echo Running $pgm
		time $pgm
	done
done
