#!/bin/bash
#
# Run a crude performance test of the various lifo-push implementations
#
# Copyright IBM Corporation, 2019
# Authors: Paul E. McKenney, IBM Linux Technology Center

for ((i=0;i<50;i++))
do
	for pgm in ./lifo-push ./lifo-push-atomic ./lifo-push-atomicw ./lifo-push-int ./lifo-push-london ./lifo-push-rcu ./lifo-push-rep
	do
		echo Running $pgm iteration $i
		time $pgm
	done
done
