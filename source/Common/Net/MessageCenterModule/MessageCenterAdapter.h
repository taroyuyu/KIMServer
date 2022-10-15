//
// Created by Kakawater on 2018/1/17.
//

#ifndef KAKAIMCLUSTER_MESSAGECENTERADAPTER_H
#define KAKAIMCLUSTER_MESSAGECENTERADAPTER_H

#include <google/protobuf/message.h>
#include "../Buffer/CircleBuffer.h"

namespace kakaIM {
    namespace net {
        class MessageCenterAdapter {
        public:
            virtual ~MessageCenterAdapter(){

            }
            /**
             * @description 将消息封装成字节流
             * @param message 待封装的消息
             * @param outputBuffer 输出缓冲区
             */
            virtual void
            encapsulateMessageToByteStream(const ::google::protobuf::Message &message, CircleBuffer & outputBuffer)const = 0;

            /**
             * 尝试从输入缓冲区中提取消息
             * @param inputBuffer 输入缓冲区
             * @param message 指向消息的指针
             * @return true表示成功提取到一条消息;false表示缓冲区中不存在一条完整的消息，即未提取到
             */
            virtual bool tryToretriveMessage(CircleBuffer & inputBuffer, ::google::protobuf::Message **message)const = 0;
        };
    }
}


#endif //KAKAIMCLUSTER_MESSAGECENTERADAPTER_H
