#/bin/bash
if [ -f agent ]
then
	rm agent
fi

if [ -f client ]
then
	rm client
fi

if [ -f sim ]
then
	rm sim
fi

g++ -static -g -W server_system_test.cc ./webinterface/mongoose.c ./plnode/protocol/plexus/golay/golay.c -lpthread -ldl -o pweb_agent &> server_output
g++ -static -g -W client_system_test.cc ./plnode/protocol/plexus/golay/golay.c -lpthread -o client &> client_output
g++ -static -g -W simulator.cc ./plnode/protocol/plexus/golay/golay.c -lpthread -ldl -o sim &> sim_output
grep error server_output
grep error client_output
grep error sim_output
rm server_output client_output sim_output

scp config sim ihostlist imonitorlist simhostlist pweb@cn101.cs.uwaterloo.ca:~/klink
ssh pweb@cn101.cs.uwaterloo.ca "cd klink; ./copy_script_src.sh"
