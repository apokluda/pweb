#!/usr/bin/expect
set timeout 100
set HOST [lindex $argv 0]
spawn ssh -l uwaterloo_pweb -i ~/.ssh/id_rsa $HOST "ssh-keygen -t rsa -N \"\" -f ~/.ssh/id_rsa"

expect {
	-re
	".*verwrite.*" {
		send "\n"
		exp_continue
	}
}

#expect {
#-re ".*Are.*.*yes.*no.*" {
#send "yes\n"
#exp_continue
#}
#"*?assword:*" {
#send "MyPaSsWoRd"
#send "\n"
#interact
#}
#}

