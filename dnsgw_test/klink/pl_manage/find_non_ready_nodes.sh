#!/bin/bash
if [ -f not_ready ]
then
	rm not_ready
fi
touch not_ready

for node in `cat pssh_nodes`
do
	found=0
	for ready in `cat ready_nodes`
	do
		if [ "$ready" == "$node" ]
		then
			found=1
			break
		fi
	done
	if test $found -eq 1
	then
		break
	else 
		echo $node >> not_ready
	fi
done
