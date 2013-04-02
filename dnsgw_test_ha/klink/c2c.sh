#!/bin/bash
for file in $*
do
	for machine in {101..110}
	do
		scp $file pweb@cn$machine:~/klink/
	done
done
