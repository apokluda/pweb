#!/bin/bash
for machine in `cat pssh_nodes`
do
	echo $machine
	scp -i ~/.ssh/id_rsa ../config_project uwaterloo_pweb@$machine:~/
done

#"scp -i ~/.ssh/id_rsa ../config uwaterloo_pweb@plink.cs.uwaterloo.ca:~/"
scp -i ~/.ssh/id_rsa ../config_project uwaterloo_pweb@plink.cs.uwaterloo.ca:~/

#for machine in `cat pssh_nodes`
#do
#	echo $machine
#	scp -i ~/.ssh/id_rsa -o StrictHostKeyChecking=no nodes ../imonitorlist ../agent ../config uwaterloo_pweb@$machine:~/
#	done=$(($done + 1))
#	echo "$done/$nodecount complete"
#done

