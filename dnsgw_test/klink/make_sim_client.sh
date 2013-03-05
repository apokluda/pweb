#!/bin/bash
if [ -f sim ]
then
	rm sim
fi
if [ -f client ]
then
	rm client
fi
g++ -g -W simulator.cc ./plnode/protocol/plexus/golay/golay.c -lpthread -ldl -o sim &> output_sim
grep error output_sim
g++ -g -W client_system_test.cc ./plnode/protocol/plexus/golay/golay.c -lpthread -o client &> output_client
grep error output_client
rm output_*
