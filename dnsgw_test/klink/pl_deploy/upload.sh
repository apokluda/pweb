#!/bin/bash
start=$1
n=$2
end=$(($start+$n-1))
#echo $end
node_seg=`head -n $end pssh_nodes | tail -n $n`
log_file="uplog_$start"
log_file=$log_file"_$end"
if [ -f $log_file ]
then
	rm $log_file
fi
#echo $node_seg
for machine in $node_seg
do
	echo $machine &>> $log_file
	scp -i ~/.ssh/id_rsa -o StrictHostKeyChecking=no nodes imonitorlist agent config uwaterloo_pweb@$machine:~/ &>> $log_file
	done=$(($done + 1))
	echo "($done/$n) complete" &>> $log_file
done

