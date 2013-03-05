/*
 * server_test.cc
 *
 *  Created on: 2012-11-24
 *      Author: sr2chowd
 */

#include "../klink/communication/server_socket.h"
#include "../klink/communication/client_socket.h"
#include "../klink/communication/socket.h"
#include "../klink/plnode/message/control/peer_config_msg.h"
#include "../klink/plnode/message/message.h"
#include "../klink/plnode/logging/log.h"
#include "../klink/plnode/logging/log_entry.h"
#include "../klink/plnode/ds/lookup_table.h"
//#include "../klink/plnode/protocol/plexus/plexus_protocol.h"
//#include "../klink/plnode/message/control/peer_init_message.h"
#include "../klink/plnode/ds/lookup_table_iterator.h"
#include "../klink/plnode/ds/overlay_id.h"

#include <vector>
#include <pthread.h>

using namespace std;
/*void *server_function( void *ptr )
 {
 ServerSocket s_socket;
 s_socket.setServerPortNumber(36020);

 int k = s_socket.listen_to_socket();

 printf("%d\n", k);

 char* buffer;
 int len = s_socket.receive_data(k, &buffer);

 s_socket.send_data(k, buffer, len);
 }*/

class K
{
	int x;
public:
	void setX(int x)
	{
		this->x = x;
	}
	int getX()
	{
		return this->x;
	}
	bool operator <(const K& k) const
	{
		return x < k.x;
	}
};

void log_test()
{
	/*Log lg("seq.txt", "latency", "localhost", "sr2chowd");
	//lg.setMonitorHostName("localhost");
	//lg.setRemoteFtpDirectory("logs");
	lg.setCheckPointRowCount(4);
	lg.open("a");

	vector <LogEntry> vLog;

	vLog.push_back(LogEntry("1", "i", 10));
	vLog.push_back(LogEntry("1", "i", 10));
	vLog.push_back(LogEntry("1", "i", 10));
	vLog.push_back(LogEntry("1", "i", 10));
	vLog.push_back(LogEntry("1", "i", 10));
	vLog.push_back(LogEntry("1", "i", 10));
	vLog.push_back(LogEntry("1", "i", 10));
	vLog.push_back(LogEntry("1", "i", 10));


	lg.write("1", "i", 10);
	lg.write("2", "i", 10);
	lg.write("3", "s", "asdf");
	lg.write("1", "i", 10);
	lg.write("4", "i", 10);
	lg.write("5", "d", 10.98);
	lg.write("6", "i", 10);
	lg.write("7", "i", 10);
	lg.write("8", "sd", "test", 10);
	lg.write("9", "i", 10);
	lg.write("10", "is", 10, "abcd");

	lg.close();
	lg.sshUploadArchive();
	//lg.ftpUploadArchive();*/
}
void lookup_table_test()
{
/*	LookupTable<K, int> table;
	K k;

	k.setX(220);
	table.add(k, 400);
	k.setX(10);
	table.add(k, 20);

	int x;
	//if(table.lookup(k, &x)) printf("%d\n", x);
	table.update(k, 30);
	//if(table.lookup(k, &x)) printf("%d\n", x);

	LookupTableIterator<K, int> table_it(&table);

	table_it.reset_iterator();
	while (table_it.hasMoreKey())
	{
		K key = table_it.getNextKey();
		printf("%d\n", key.getX());
	}

	table.remove(k);
	vector<K> a = table.getKeySet();
	printf("%d\n", (int) a.size());*/
}

/*void plexus_protocol_test()
 {
 PlexusProtocol* pr = new PlexusProtocol();
 }*/

/*void message_test()
 {
 PeerInitMessage* pInit = new PeerInitMessage();
 LookupTable <OverlayID, HostAddress> rTable;

 OverlayID key;

 key.SetOverlay_id(1000);
 HostAddress value;

 value.SetHostName("localhost");
 value.SetHostPort(123);
 rTable.add(key, value);

 key.SetOverlay_id(333);
 value.SetHostName("otherhost");
 value.SetHostPort(1234);
 rTable.add(key, value);

 pInit->setRoutingTable(rTable);
 pInit->message_print_dump();

 char* buffer;
 int buffer_length;

 puts("Serializing..");
 buffer = pInit->serialize(&buffer_length);
 puts("Serializing Complete..");

 PeerInitMessage* other = new PeerInitMessage();
 other->deserialize(buffer, buffer_length);

 other->message_print_dump();

 }*/

void rm_test()
{
	int hash_name_to_publish = 5;

	for(int i = 0; i < 10; i++)
	{
		OverlayID destID(hash_name_to_publish);

		cout << "id = " << hash_name_to_publish << ends << " odi = ";
		destID.printBits();
		cout << endl;
	}
}
int main()
{
	//server_function(NULL);
	//log_test();
	//message_test();
	//lookup_table_test();
	rm_test();
	return 0;
}

