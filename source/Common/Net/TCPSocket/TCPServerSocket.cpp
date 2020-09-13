//
// Created by taroyuyu on 2018/1/3.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <zconf.h>
#include <csignal>
#include <string.h>
#include "TCPServerSocket.h"

namespace kakaIM
{
    namespace net{
        TCPServerSocket::TCPServerSocket(std::string addr, unsigned short port,SocketOptionState tcpNoDelay,SocketOptionState reuseAddr)
        {
            this->m_addr = addr;
            this->m_port = port;
            this->m_socketfd = socket(AF_INET, SOCK_STREAM,IPPROTO_TCP);

            //配置socket
            char tcpNoDelayState = 1;
            if (Off == tcpNoDelay){
                tcpNoDelayState = 0;
            }
            setsockopt(this->m_socketfd,IPPROTO_TCP,TCP_NODELAY,(void*)&tcpNoDelayState, sizeof(tcpNoDelayState));

            char reuseAddrState = 1;
            if (Off == reuseAddr){
                reuseAddrState = 0;
            }
            setsockopt(this->m_socketfd,SOL_SOCKET,SO_REUSEADDR,(void*)&reuseAddrState, sizeof(reuseAddrState));

            //绑定地址
            struct sockaddr_in listenAddr;
            memset(&listenAddr, 0, sizeof(listenAddr));
            listenAddr.sin_family = AF_INET;
            listenAddr.sin_addr.s_addr = inet_addr(this->m_addr.c_str());
            listenAddr.sin_port = htons(this->m_port);


            if(-1 == bind(this->m_socketfd, (struct sockaddr *) &listenAddr, sizeof(listenAddr))){
                //关闭socket
                ::close(this->m_socketfd);
                //抛出异常
            }
        }

        void TCPServerSocket::listen(int backlog) {
            if (-1 == ::listen(this->m_socketfd,backlog)){

            }else{
                //忽略SIGPIPE信号
                signal(SIGPIPE, SIG_IGN);
            }
        }

        TCPClientSocket TCPServerSocket::accept(){
            int clientSocketfd = -1;
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            clientSocketfd = ::accept(this->m_socketfd, (struct sockaddr *) &clientAddr, &clientAddrLen);
            TCPClientSocket clientSocket(clientSocketfd);
            return clientSocket;
        }

        void TCPServerSocket::close() {
            ::close(this->m_socketfd);
        }
    }
}