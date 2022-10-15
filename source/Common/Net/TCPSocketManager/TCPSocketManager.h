//
// Created by Kakawater on 2018/1/10.
//

#ifndef KAKAIMCLUSTER_TCPSOCKETMANAGER_H
#define KAKAIMCLUSTER_TCPSOCKETMANAGER_H

#include <functional>
#include "../TCPSocket/TCPClientSocket.h"
#include "TCPSocketManaerAdapter.h"
#include "TCPSocketManagerConsignor.h"
#include <map>
#include <list>
#include <thread>
#include <google/protobuf/message.h>
#include <memory>

namespace kakaIM {
    namespace net {

        class CircleBuffer;

        class TCPSocketManager {
        public:
            TCPSocketManager();
            ~TCPSocketManager();

            void start();

            void stop();

            void addSocket(int socketfd, TCPSocketManagerConsignor *consignor,TCPSocketManaerAdapter * adapter);

            void addSocket(TCPClientSocket &socket, TCPSocketManagerConsignor *consignor,TCPSocketManaerAdapter * adapter);

            void sendMessage(TCPClientSocket &socket, const ::google::protobuf::Message & message);

            void sendMessage(int socketfd, const ::google::protobuf::Message & message);

        private:
            void execute();


            void dispatchMessage(int socketfd,const ::google::protobuf::Message & message);

            void closeConnection(int socketfd);

            std::thread m_workThread;
            bool m_isStarted;
            enum ProtocolVersion {
                KakaIMClusterProto_1 = 1//版本1
            };
            struct KakaIMClusterDatagram {
                u_int8_t protocolVersion;//协议版本 占1个字节
                u_int8_t messageType;//消息类型     占1个字节
                u_int32_t len;//数据长度            占4个字节
            };
            int mEpollInstance;
            std::list<int> mSocketTable;
            std::map<int,std::pair<TCPSocketManagerConsignor *,TCPSocketManaerAdapter*>> mConnectionConsignorTable;
            /**
             * socket输入缓冲区

             */
            std::map<int,std::unique_ptr<CircleBuffer>> mSocketInputBufferMap;
            /**
             * socket输出缓冲区
             */
            std::map<int,std::unique_ptr<CircleBuffer>> mSocketOutputBufferMap;
        };
    }
}


#endif //KAKAIMCLUSTER_TCPSOCKETMANAGER_H
