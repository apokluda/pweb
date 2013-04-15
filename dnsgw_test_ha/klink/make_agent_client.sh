#!/bin/bash
if [ -f agent_project ]
then
	rm agent_project
fi
if [ -f client_project ]
then
	rm client_project
fi
g++ -g -W server_system_test.cc ./webinterface/mongoose.c ./plnode/protocol/plexus/golay/golay.c -lpthread -ldl -o agent_project &> output_agent
grep error output_agent
g++ -g -W client_system_test.cc ./plnode/protocol/plexus/golay/golay.c -lpthread -o client_project &> output_client
grep error output_client
rm output_*
