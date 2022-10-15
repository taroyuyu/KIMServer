//
// Created by Kakawater on 2018/1/25.
//
#include "../../Common/proto/KakaIMMessage.pb.h"

#ifndef KAKAIMCLUSTER_MESSAGEPERSISTENCESERVICE_H
#define KAKAIMCLUSTER_MESSAGEPERSISTENCESERVICE_H
namespace kakaIM {
    namespace node {
        class MessagePersistenceService {
        public:
            /**
             * 将消息持久化
             * @param message 待持久化的消息
             * @param messageID 消息ID
             */
            virtual void persistChatMessage(std::string userAccount, const kakaIM::Node::ChatMessage &message,
                                            const uint64_t messageID)  = 0;

            /**
                 * 将群消息进行持久化
                 * @param groupChatMessage
                 * @param messageID
                 */
            virtual void persistGroupChatMessage(const kakaIM::Node::GroupChatMessage &groupChatMessage,
                                                 const uint64_t messageID) = 0;

        };
    }
}
#endif //KAKAIMCLUSTER_MESSAGEPERSISTENCESERVICE_H
