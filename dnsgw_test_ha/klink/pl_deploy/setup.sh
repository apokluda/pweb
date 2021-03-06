#!/bin/bash
./find_non_ready_nodes.sh
./all_node_idgen.sh
./all_node_idcopy.sh
cat not_ready >> ready_nodes
if [ -f nodes_temp ]
then
	rm nodes_temp
fi

if [ -f nodes_project ]
then
	rm nodes_project
fi

touch nodes_project
nodecount=`wc -l pssh_nodes | cut -d' ' -f 1`
done=0
echo $nodecount >> nodes_project
for machine in `cat pssh_nodes`
do
	ip=`host $machine | egrep '[[:digit:]]{1,3}\.[[:digit:]]{1,3}\.[[:digit:]]{1,3}\.[[:digit:]]{1,3}' | cut -d' ' -f 4`
	echo "$machine 45231 $ip" >> nodes_temp
done
cat nodes_temp | awk '{len=split($1,array,"."); print $1, $2, $3, array[len-1] map[array[len-1]]++;}' >> nodes_project

scp pssh_nodes ../config_project ../imonitorlist_project ../agent_project nodes_project pweb@cn102.cs.uwaterloo.ca:~/klink/pl_deploy
ssh pweb@cn102.cs.uwaterloo.ca "cd klink/pl_deploy ; ./upload_wrap.sh &"
scp -i ~/.ssh/id_rsa ../config_project ../imonitorlist_project nodes_project uwaterloo_pweb@plink.cs.uwaterloo.ca:~/

#for machine in `cat pssh_nodes`
#do
#	echo $machine
#	scp -i ~/.ssh/id_rsa -o StrictHostKeyChecking=no nodes ../imonitorlist ../agent ../config uwaterloo_pweb@$machine:~/
#	done=$(($done + 1))
#	echo "$done/$nodecount complete"
#done

