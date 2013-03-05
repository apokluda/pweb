#ifndef HOST_ADDRESS_H
#define HOST_ADDRESS_H

#include <cmath>
#include <cstring>

using namespace std;

class HostAddress {
private:
        string hostName;
        int hostPort;
public:

        HostAddress() {
                hostName = "";
                hostPort = -1;
        }

        HostAddress(string name, int port) {
                hostName = name;
                hostPort = port;
        }

        void SetHostPort(int hostPort) {
                this->hostPort = hostPort;
        }

        int GetHostPort() const {
                return hostPort;
        }

        void SetHostName(string hostName) {
                this->hostName = hostName;
        }

        string GetHostName() const {
                return hostName;
        }

        bool operator<(const HostAddress& addr) const {
                int minLength = this->hostName.size();
                if (minLength > addr.hostName.size())
                        minLength = addr.hostName.size();

                if (strncmp(this->hostName.c_str(), addr.hostName.c_str(), minLength) < 0) {
                        return true;
                } else return this->hostPort < addr.hostPort;
        }

        bool operator ==(const HostAddress& addr) {
                int minLength = this->hostName.size();
                if (minLength > addr.hostName.size())
                        minLength = addr.hostName.size();

                return (strncmp(this->hostName.c_str(), addr.hostName.c_str(), minLength) == 0 &&
                        this->hostPort == addr.hostPort);
        }

        char* toString() {
                char* result = new char[getStringSize() + 1];
                sprintf(result, "%s:%d", hostName.c_str(), hostPort);
                result[getStringSize()] = '\0';
                return result;
        }

        unsigned numDigits(unsigned number) {
                int digits = 0;
                if (number == 0) return 1;
                while (number != 0) {
                        number /= 10;
                        digits++;
                }
                return digits;
        }

        int getStringSize() {
                return hostName.size() + 1 + numDigits(hostPort);
        }
};
#endif
