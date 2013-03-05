#!/bin/bash
for host in `cat not_ready`
do
	./single_node_idgen.sh $host
done
