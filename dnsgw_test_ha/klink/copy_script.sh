#/bin/bash
basename="cn"
for i in {101..105}
do
	cluster=$basename$i.cs.uwaterloo.ca
	echo $cluster
	scp agent client ihostlist pweb@$cluster:~/klink
	#scp client_system_test server_system_test pweb@$cluster:~/klink
done
