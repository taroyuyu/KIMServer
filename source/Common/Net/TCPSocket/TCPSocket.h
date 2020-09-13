//
// Created by taroyuyu on 2018/1/3.
//

#ifndef KAKAIMCLUSTER_TCPSOCKET_H
#define KAKAIMCLUSTER_TCPSOCKET_H

#include <string>

namespace kakaIM {
    namespace net {
        class TCPSocket {
        public:
            ~TCPSocket(){

            }
            enum SocketOptionState{
                Off,//关闭
                On,//开启
            };
            virtual void close() = 0;
            int getSocketfd(){
                return m_socketfd;
            }
        protected:
            int m_socketfd;
            std::string m_addr;
            unsigned short m_port;
        };
    }
}

#endif //KAKAIMCLUSTER_TCPSOCKET_H
