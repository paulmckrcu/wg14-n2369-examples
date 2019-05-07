#!/bin/sh
#
# Usage: capcpufreq.sh MHz
#
# Sets the maximum CPU frequency to the specified number of MHz.
# Usually needs to be run as root.  Designed for Ubuntu, so your
# mileage on other distributions (to say nothing of OSes) may vary.

for x in /sys/devices/system/cpu/*/cpufreq/
do
	echo ${1}000 > $x/scaling_max_freq
done
