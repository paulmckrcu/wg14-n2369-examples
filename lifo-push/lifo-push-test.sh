#!/bin/bash
#
# Run a crude performance test of the various lifo-push implementations
#
# Copyright IBM Corporation, 2019
# Authors: Paul E. McKenney, IBM Linux Technology Center

ret=0
for ((i=0;i<50;i++))
do
	for pgm in ./lifo-push ./lifo-push-atomic ./lifo-push-atomicw ./lifo-push-int ./lifo-push-london ./lifo-push-rcu ./lifo-push-rep
	do
		echo Running $pgm iteration $i
		if time $pgm
		then
			:
		else
			echo "!!! Run failed"
			ret=1
		fi
	done
done
exit $ret
