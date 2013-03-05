#!/bin/bash

#T="$(date +%s%N)"
parallel-ssh -h pssh_nodes_temp -x "-l uwaterloo_pweb" -x "-i ~/.ssh/id_rsa" hostname &> pssh.log
grep 'SUCCESS' pssh.log | cut -d' ' -f 4 > pssh_nodes
rm pssh.log
# Time interval in nanoseconds
#T="$(($(date +%s%N)-T))"
# Seconds
#S="$((T/1000000000))"
# Milliseconds
#M="$((T/1000000))"

#printf "Time elasped: %02dm:%02d.%03ds\n" "$((S/60%60))" "$((S%60))" "${M}"


