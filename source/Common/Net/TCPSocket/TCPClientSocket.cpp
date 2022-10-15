//
// Created by Kakawater on 2018/1/3.
//

#include <netinet/in.h>
#include <zconf.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cstring>
#include "TCPClientSocket.h"

namespace kakaIM{
    namespace net{
        TCPClientSocket::TCPClientSocket(const std::string & localAddr,const int localPort)
        {
            this->m_socketfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
            int value = 1;
            setsockopt(this->m_socketfd, SOL_SOCKET,MSG_NOSIGNAL, &value, sizeof(value));
            if (localAddr != "" && localPort > 0){
                sockaddr_in localAddr_in;
                memset(&localAddr_in,0, sizeof(localAddr_in));

                localAddr_in.sin_family = AF_INET;
                localAddr_in.sin_addr.s_addr = inet_addr(localAddr.c_str());
                localAddr_in.sin_port = htons(localPort);

                if(-1 == bind(this->m_socketfd, (struct sockaddr *) &localAddr_in, sizeof(localAddr_in))){
                    //关闭socket
                    ::close(this->m_socketfd);
                    //抛出异常
                    perror(__FUNCTION__);
                    exit(-1);
                }else{
                    this->m_addr = localAddr;
                    this->m_port = localPort;
                }
            }
        }
        TCPClientSocket::TCPClientSocket(const TCPClientSocket & clientSocket)
        {
            this->m_socketfd = clientSocket.m_socketfd;
            int value = 1;
            setsockopt(this->m_socketfd, SOL_SOCKET,MSG_NOSIGNAL, &value, sizeof(value));
        }
        TCPClientSocket::TCPClientSocket(int socketfd)
        {
            this->m_socketfd = socketfd;
            struct sockaddr_in addr;
            socklen_t len = sizeof(struct sockaddr_in);
            memset(&addr,0,sizeof(addr));
            ::getsockname(m_socketfd, (struct sockaddr*)&addr, &len);
            this->m_addr = addr.sin_addr.s_addr;
            this->m_port = addr.sin_port;

        }

        bool TCPClientSocket::operator==(const TCPClientSocket &socket){
            return this->m_socketfd == socket.m_socketfd;
        }
        void TCPClientSocket::connect(const std::string serverAddr,const int serverPort,SocketOptionState tcpNoDelay)
        {
            //配置socket
            char tcpNoDelayState = 1;
            if (Off == tcpNoDelay){
                tcpNoDelayState = 0;
            }
            setsockopt(this->m_socketfd,IPPROTO_TCP,TCP_NODELAY,(void*)&tcpNoDelayState, sizeof(tcpNoDelayState));

            sockaddr_in serverAddr_in;
            memset(&serverAddr_in,0, sizeof(serverAddr_in));

            serverAddr_in.sin_family = AF_INET;
            serverAddr_in.sin_addr.s_addr = inet_addr(serverAddr.c_str());
            serverAddr_in.sin_port = htons(serverPort);

            if(-1 == ::connect(this->m_socketfd,(struct sockaddr*)&serverAddr_in, sizeof(serverAddr_in))){
                //连接出错,抛出异常
                perror(__func__);
                exit(-1);
            }else{
                struct sockaddr_in localAddr_in;
                memset((void*)&localAddr_in,0, sizeof(localAddr_in));
                socklen_t  localAddr_in_len = 0;
                if(0 == ::getsockname(this->m_socketfd,(struct sockaddr*)&localAddr_in,&localAddr_in_len)){
                    this->m_addr = localAddr_in.sin_addr.s_addr;
                    this->m_port = localAddr_in.sin_port;
                }
            }
        };

        void TCPClientSocket::close()
        {
            ::close(this->m_socketfd);
        }

        int TCPClientSocket::send(char * buffer, long bufferLength)
        {
            return write(this->m_socketfd,buffer,bufferLength);
        }
    }
}
