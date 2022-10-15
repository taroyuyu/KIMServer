//
// Created by Kakawater on 2018/1/31.
//

#ifndef KAKAIMCLUSTER_KAKAIMCONFIGS_H
#define KAKAIMCLUSTER_KAKAIMCONFIGS_H

#include <string>
namespace kakaIM {
    namespace client {
        class KakaIMConfigs {
        public:
            void setServerAddr(const std::string serverAddr){
                this->serverAddr = serverAddr;
            }
            void setServerPort(const uint32_t serverPort){
                this->serverPort = serverPort;
            }
            std::string getServerAddr()const{
                return this->serverAddr;
            }
            uint32_t getServerPort()const{
                return this->serverPort;
            }
        private:
            std::string serverAddr;
            uint32_t serverPort;
        };
    }
}


#endif //KAKAIMCLUSTER_KAKAIMCONFIGS_H
