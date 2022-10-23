//
// Created by Kakawater on 2018/1/26.
//

#ifndef KAKAIMCLUSTER_MESSAGESENDSERVICEMODULE_H
#define KAKAIMCLUSTER_MESSAGESENDSERVICEMODULE_H

#include <Node/KIMNodeModule/KIMNodeModule.h>
#include <functional>

namespace kakaIM {
    namespace node {
        class MessageSendServiceModule
                : public KIMNodeModule, public MessageSendService, public ClusterService::ServerMessageListener {
        public:
            MessageSendServiceModule();

            /**
             * 发送消息给特定的用户
             * @param userAccount 用户账号
             * @param message 待发送的消息
             * @return
             */
            virtual void
            sendMessageToUser(const std::string &userAccount, const ::google::protobuf::Message &message) override;

            virtual void
            sendMessageToSession(const std::string &serverID, const std::string &sessionID,
                                 const ::google::protobuf::Message &message) override;

            virtual void
            didReceivedServerMessageFromCluster(const kakaIM::president::ServerMessage &serverMessage) override;

            virtual void setClusterService(std::weak_ptr<ClusterService> service) override;
            virtual const std::unordered_set<std::string> messageTypes() override;
        protected:
            virtual void execute() override;
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>,const std::string> &task) override;
            std::unordered_map<std::string,std::function<void(std::unique_ptr<::google::protobuf::Message>, const std::string)>> mMessageHandlerSet;
        private:
            ConcurrentLinkedQueue<kakaIM::president::ServerMessage> mServerMessageQueue;

            void dispatchServerMessage(kakaIM::president::ServerMessage &message);

            void handleMessageForSend(const std::string &userAccount, ::google::protobuf::Message &message);

            void handleSessionMessage(kakaIM::president::SessionMessage &message);

            void handleServerMessageFromCluster(const kakaIM::president::ServerMessage &serverMessage);
        };
    }
}


#endif //KAKAIMCLUSTER_MESSAGESENDSERVICEMODULE_H
