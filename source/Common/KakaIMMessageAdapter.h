//
// Created by Kakawater on 2018/1/18.
//

#ifndef KAKAIMCLUSTER_KAKAIMMESSAGEADAPTER_H
#define KAKAIMCLUSTER_KAKAIMMESSAGEADAPTER_H

#include "Net/MessageCenterModule/MessageCenterAdapter.h"
#include "Net/TCPSocketManager/TCPSocketManaerAdapter.h"
namespace kakaIM {
    namespace common {
        class KakaIMMessageAdapter: public net::MessageCenterAdapter, public net::TCPSocketManaerAdapter {
        public:
            virtual  void encapsulateMessageToByteStream(const ::google::protobuf::Message & message,net::CircleBuffer & outputBuffer)const  override ;
            virtual bool tryToretriveMessage(net::CircleBuffer & inputBuffer,::google::protobuf::Message ** message)const override ;
        };
    }
}


#endif //KAKAIMCLUSTER_KAKAIMMESSAGEADAPTER_H
