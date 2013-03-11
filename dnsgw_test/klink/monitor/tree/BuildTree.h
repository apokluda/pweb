#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>

#include "../../plnode/ds/lookup_table.h"
#include "../../plnode/ds/lookup_table_iterator.h"
#include "../../plnode/ds/overlay_id.h"
#include "../../plnode/ds/host_address.h"
#include "../../plnode/ds/GlobalData.h"
#include "../../plnode/protocol/code.h"

class BuildTree {
public:
        string fileName;
        int treeSize;
        OverlayID *idArray;
        HostAddress* hAddArray;
	string* aliasArray;
        LookupTable<OverlayID, HostAddress> *rtArray;
        LookupTable<OverlayID, HostAddress> *hosts;
        int max_height;
        ABSCode *iCode;

        BuildTree(string fileName, ABSCode *iCode);
        int GetHeight(int index);
        int GetIndexOfLongestMatchedPrefix(OverlayID id);

        LookupTable<OverlayID, HostAddress>& getRoutingTablePtr(int index) {
                return rtArray[index];
        }

        OverlayID& getOverlayID(int index) {
                return idArray[index];
        }

        int getTreeSize() const {
                return treeSize;
        }

        HostAddress getHostAddress(int index) {
                return hAddArray[index];
        }
        void execute();
        void print();

        int GetBitAtPosition(int value, int n) const {
                return (((value & (1 << n)) >> n) & 0x00000001);
        }

        void printBits(int value, int length) {
                for (int i = length - 1; i >= 0; i--) {
                        cout << GetBitAtPosition(value, i);
                }
        }

};

BuildTree::BuildTree(string fileName, ABSCode *iCode) {
        this->fileName = fileName;
        this->iCode = iCode;
}

int BuildTree::GetHeight(int index) {
        if (index < (pow(2.0, max_height) - treeSize))
                return max_height - 1;
        return max_height;
}

int BuildTree::GetIndexOfLongestMatchedPrefix(OverlayID id) {
        int maxLength = 0, length;
        int maxIndex = 0;
        for (int i = 0; i < treeSize; i++) {
                length = id.GetMatchedPrefixLength(idArray[i]);
                if (length > maxLength) {
                        maxLength = length;
                        maxIndex = i;
                }
        }
        return maxIndex;
}

void BuildTree::execute() {
        //open host list file
        ifstream hostListFile(fileName.c_str());
        string line;

        if (hostListFile.is_open()) {
                // Initialize local variables
        	    std::getline(hostListFile, line);
        	    this->treeSize = atoi(line.c_str());

                //hostListFile >> this->treeSize;
                this->idArray = new OverlayID[treeSize];
                this->hAddArray = new HostAddress[treeSize];
		this->aliasArray = new string[treeSize];
                this->rtArray = new LookupTable<OverlayID, HostAddress> [treeSize];
                this->hosts = new LookupTable<OverlayID, HostAddress> [treeSize];
                this->max_height = ceil(log2(treeSize));

                //rm code
                //rm = new ReedMuller(2, 4);
                //cout << "k = " << rm->rm->k << endl;
                //cout << "n = " << rm->rm->n << endl;

                //read file and generate overlayid, hostname and port data
                string hostName;
		string ip_address;
                int hostPort;
		string alias;
                //the following variable are required for method2 of overlay id generation
                int pattern = 0;
                int nodesAtMaxHeight = 2 * treeSize - pow(2.0, max_height);
                int nodesAtPrevLevel = treeSize - nodesAtMaxHeight;
                int startingNodeAtPrevLevel = pow(2.0, max_height - 1)
                        - nodesAtPrevLevel;
                int st = startingNodeAtPrevLevel;
                for (int i = 0; i < treeSize; i++) {
                		line = "";
                        //hostname and port
                	    std::getline(hostListFile, line);

                	    char buf[400];
                	    strcpy(buf, line.c_str());
                	    printf("line >> %s\n", buf);

                	    char* token = strtok(buf, " \n");
                	    hostName = string(token);

                	    token = strtok(NULL, " \n");
                	    hostPort = atoi(token);

			token = strtok(NULL, " \n");
			ip_address = string(token);

			token = strtok(NULL, " \n");
			alias = string(token);
			
			aliasArray[i] = alias;

                        //hostListFile >> hostName;
                        //hostListFile >> hostPort;
                        HostAddress ha;
                        ha.SetHostName(hostName);
                        ha.SetHostPort(hostPort);
                        hAddArray[i] = ha;

                        //generate overlayids
                        //method 1: not in use
                        //idArray[i] = OverlayID(rm->array2int(rm->encode(rm->int2array(i, rm->rm->k)), rm->rm->n), GetHeight(i), rm->rm->n);
                        //method 2: complicated but makes forwarding easier
                        pattern = (((i < nodesAtMaxHeight) ? i : st++)
                                << (iCode->K()
                                - ((i < nodesAtMaxHeight) ?
                                max_height : (max_height - 1))));
                        //cout << "Pattern = ";
                        //printBits(pattern, rm->rm->k);
                        //cout << " ";
                        //idArray[i] = OverlayID(rm->array2int(rm->encode(rm->int2array(pattern, rm->rm->k)), rm->rm->n), (i < nodesAtMaxHeight) ? max_height : (max_height - 1), rm->rm->n);
                        idArray[i] = OverlayID(pattern,
                                (i < nodesAtMaxHeight) ? max_height : (max_height - 1), iCode);
                        //idArray[i].printBits();
                        //cout << " pl = " << idArray[i].GetPrefix_length() << endl;

                        //save in map for later retrieval
                        hosts->add(idArray[i], hAddArray[i]);
                }
                hostListFile.close();

                //build routing table
                ///////////////////////////////////////////////////////////////////////////
                
                int nbrIndex;
                cout << "OID MAX LENGHT " << idArray[0].MAX_LENGTH << endl;
                for (int i = 0; i < this->treeSize; i++)
                {
                        rtArray[i] = LookupTable<OverlayID, HostAddress>();
                        //toggle each bit (upto prefix length) and find the nbr
                        OverlayID replica = idArray[i];
                        printf("Prefix Length = %d\n", idArray[i].GetPrefix_length());

                        for (int j = 0; j < idArray[i].GetPrefix_length(); j++)
                        {
                                OverlayID nbrPattern = idArray[i].ToggleBitAtPosition(
                                                idArray[i].MAX_LENGTH - j - 1);
                                replica = replica.ToggleBitAtPosition(
                                                idArray[i].MAX_LENGTH - j - 1);
                                nbrIndex = GetIndexOfLongestMatchedPrefix(nbrPattern);
                                if (idArray[nbrIndex] != idArray[i])
                                {
                                        HostAddress ha;
                                        hosts->lookup(idArray[nbrIndex], &ha);
                                        rtArray[i].add(idArray[nbrIndex], ha);
                                }
                        }
                        //            cout << "CW = ";
                        //            idArray[i].printBits();
                        //            cout << " pl = " << idArray[i].GetPrefix_length() << " replica = ";
                        //            replica.printBits();
                        //            cout << endl;
                        //add link to replica :: toggled all bits (replica)
                        nbrIndex = GetIndexOfLongestMatchedPrefix(replica);
                        if (idArray[nbrIndex] != idArray[i])
                        {
                                HostAddress ha;
                                hosts->lookup(idArray[nbrIndex], &ha);
                                rtArray[i].add(idArray[nbrIndex], ha);
                        }
                }
                /////////////////////////////////////////////////////////////////////
                 

//                int nbrIndex;
//                cout << "OID MAX LENGHT " << idArray[0].MAX_LENGTH << endl;
//                for (int i = 0; i < this->treeSize; i++) {
//                        rtArray[i] = LookupTable<OverlayID, HostAddress > ();
//                        for (int j = 0; j < treeSize; j++) {
//                                if(i==j) continue;
//                                HostAddress ha;
//                                hosts->lookup(idArray[j], &ha);
//                                rtArray[i].add(idArray[j], ha);
//                        }
//
//                }
        } else
                cout << "ERROR: opening host list file.";
}

void BuildTree::print() {
        cout << "k = " << iCode->K() << " n = " << iCode->N() << endl;
        for (int i = 0; i < treeSize; i++) {
                cout << "CW = ";
                idArray[i].printBits();
                cout << " pl = " << idArray[i].GetPrefix_length();
                cout << endl << "RT" << endl;

                LookupTableIterator<OverlayID, HostAddress> rtable_itr(&rtArray[i]);
                rtable_itr.reset_iterator();
                while (rtable_itr.hasMoreKey()) {
                        OverlayID key = rtable_itr.getNextKey();
                        key.printBits();
                        cout << endl;
                }

                /*vector<OverlayID> keys = rtArray[i].getKeySet();
                 for (int j = 0; j < keys.size(); j++) {
                 keys[j].printBits();
                 cout << endl;
                 }*/
                cout << "==================================" << endl;
        }
}

