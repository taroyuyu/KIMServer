//
// Created by Kakawater on 2018/2/25.
//

#ifndef KAKAIMCLUSTER_NODE_H
#define KAKAIMCLUSTER_NODE_H

#include <string>

namespace kakaIM {
    namespace president {
        class Node {
        public:
            Node(const std::string serverID, const std::string serverConnectionIdentifier, std::string serverIpAddress,uint16_t serverPort,std::pair<float,float> serverLngLatPair,std::string serviceAddr,uint16_t servicePort)
                    : serverID(serverID),
                      serverConnectionIdentifier(serverConnectionIdentifier), serverIpAddress(serverIpAddress),serverPort(serverPort),serverLngLatPair(serverLngLatPair),serviceIpAddress(serviceAddr),servicePort(servicePort){
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
	    std::string getServiceIpAddress()const{
                return this->serviceIpAddress;
            }
            uint16_t getServicePort()const{
                return this->servicePort;
            }
        private:
            std::string serverID;
            std::string serverConnectionIdentifier;
            std::string serverIpAddress;
            uint16_t serverPort;
            std::pair<float,float> serverLngLatPair;
            std::string serviceIpAddress;
            uint16_t servicePort;
        };
    }
}
#endif //KAKAIMCLUSTER_NODE_H


