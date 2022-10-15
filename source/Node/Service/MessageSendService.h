//
// Created by Kakawater on 2018/1/25.
//

#ifndef KAKAIMCLUSTER_MESSAGESENDSERVICE_H
#define KAKAIMCLUSTER_MESSAGESENDSERVICE_H

#include <string>

namespace kakaIM {
    namespace node {
        class MessageSendService {
        public:
            /**
             * 发送消息给特定的用户
             * @param userAccount 用户账号
             * @param message 待发送的消息
             * @return
             */
            virtual void
            sendMessageToUser(const std::string &userAccount, const ::google::protobuf::Message &message) = 0;

            /**
             * 发送消息给指定的会话
             * @param serverID
             * @param sessionID
             * @param message
             */
            virtual void
            sendMessageToSession(const std::string &serverID,const std::string & sessionID, const ::google::protobuf::Message &message) = 0;
        };

    }
}
#endif //KAKAIMCLUSTER_MESSAGESENDSERVICE_H
