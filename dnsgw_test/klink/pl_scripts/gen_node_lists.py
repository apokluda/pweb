import sys
import xmlrpclib
import socket
import subprocess

node_count = 200

api_server = xmlrpclib.ServerProxy('https://www.planet-lab.org/PLCAPI/')
auth = {}
auth['Username'] = "mfbari@uwaterloo.ca"
# always 'password', for password based authentication 
auth['AuthMethod'] = "password"
auth['AuthString'] = "fozU0099" 
# valid roles include, 'user', 'tech', 'pi', 'admin'
auth['Role']= "user"
slice_name = 'uwaterloo_pweb'


#print "Get current node list ..."
current_node_ids = api_server.GetSlices(auth, [slice_name], ['node_ids'])[0]['node_ids']
current_node_list = [node['hostname'] for node in api_server.GetNodes(auth, current_node_ids, ['hostname'])]
#print 'Number of nodes in slice ', len(current_node_list)

current_count = 0
node_list = []
for node in current_node_list:
	if current_count == node_count:
		break
	#print 'hostname: ', node
	if node.endswith('.edu') or node.endswith('.ca'):
		ping = subprocess.Popen(["ping", "-c", "1", node], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
		out, error = ping.communicate()
		if ", 0 received" in out or len(error) > 0:
			continue	
		node_list.append(node)	
		#print 'node_list size:', len(node_list)
		current_count += 1

f_nodes = open('nodes', 'w')
f_pssh = open('pssh_nodes_temp', 'w')
for node in node_list:
	f_nodes.write(node + " 20000" + "\n")
	f_pssh.write(node + "\n")
	f_nodes.flush()
	f_pssh.flush()
f_nodes.close()
f_pssh.close()
#print "done."
