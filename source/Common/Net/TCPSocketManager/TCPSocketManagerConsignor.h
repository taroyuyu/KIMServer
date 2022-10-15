//
// Created by Kakawater on 2018/1/10.
//

#ifndef KAKAIMCLUSTER_TCPSOCKETMANAGERCONSIGNOR_H
#define KAKAIMCLUSTER_TCPSOCKETMANAGERCONSIGNOR_H

#include <google/protobuf/message.h>
#include "../TCPSocket/TCPClientSocket.h"

namespace kakaIM {
    namespace net {
        class TCPSocketManager;

        class TCPSocketManagerConsignor {
            friend class TCPSocketManager;
        public:
            virtual ~TCPSocketManagerConsignor(){

            }
        protected:
            virtual void didConnectionClosed(TCPClientSocket clientSocket) = 0;

            virtual void didReceivedMessage(int socketfd, const ::google::protobuf::Message &) = 0;
        };
    }
}


#endif //KAKAIMCLUSTER_TCPSOCKETMANAGERCONSIGNOR_H
