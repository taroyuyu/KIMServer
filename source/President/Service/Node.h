//
// Created by taroyuyu on 2018/2/25.
//

#ifndef KAKAIMCLUSTER_NODE_H
#define KAKAIMCLUSTER_NODE_H

#include <string>

namespace kakaIM {
    namespace president {
        class Node {
        public:
            Node(const std::string serverID, const std::string serverConnectionIdentifier, std::string serverIpAddress,uint16_t serverPort,std::pair<float,float> serverLngLatPair)
                    : serverID(serverID),
                      serverConnectionIdentifier(serverConnectionIdentifier), serverIpAddress(serverIpAddress),serverPort(serverPort),serverLngLatPair(serverLngLatPair){
            }

            bool operator<(const Node &node) const {
                return this->serverID < node.serverID;
            }

            std::string getServerID() const {
                return this->serverID;
            }

            std::string getServerConnectionIdentifier() const {
                return this->serverConnectionIdentifier;
            }

            std::string getServerIpAddress()const{
                return this->serverIpAddress;
            }
            
            uint16_t getServerPort()const{
                return this->serverPort;
            }

            std::pair<float,float> getServerLngLatPair()const{
                return this->serverLngLatPair;
            };
        private:
            std::string serverID;
            std::string serverConnectionIdentifier;
            std::string serverIpAddress;
            uint16_t serverPort;
            std::pair<float,float> serverLngLatPair;
        };
    }
}
#endif //KAKAIMCLUSTER_NODE_H


