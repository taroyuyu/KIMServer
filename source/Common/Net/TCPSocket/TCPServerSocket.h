//
// Created by taroyuyu on 2018/1/3.
//

#ifndef KAKAIMCLUSTER_SOCKET_H
#define KAKAIMCLUSTER_SOCKET_H

#include "TCPSocket.h"
#include "TCPClientSocket.h"
namespace kakaIM {
    namespace net {
        class TCPServerSocket: public TCPSocket {
        public:
            TCPServerSocket(std::string addr, unsigned short port,SocketOptionState tcpNoDelay = On,SocketOptionState reuseAddr = On);
            TCPServerSocket(TCPServerSocket &) = delete;
            void listen(int backlog = 20);
            TCPClientSocket accept();
            virtual void close()override ;
        };
    }
}


#endif //KAKAIMCLUSTER_SOCKET_H
