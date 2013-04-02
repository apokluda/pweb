#!/bin/bash
nodecount=`wc -l pssh_nodes | cut -d' ' -f 1`
start=1
seg_size=30
n=$nodecount
while [ $n -gt 0 ]
do
	n=$(($n-$seg_size))
	if [ $n -lt 0 ]
	then
		seg_size=$(($seg_size+$n))
	fi
	./upload.sh $start $seg_size &
	start=$(($start+$seg_size))
done
