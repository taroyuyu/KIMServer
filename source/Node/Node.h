//
// Created by taroyuyu on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_NODE_H
#define KAKAIMCLUSTER_NODE_H

#include <semaphore.h>
#include <memory>
#include <string>
#include "Service/ConnectionOperationService.h"
#include "Service/NodeConnectionQueryService.h"
#include "../Common/Net/MessageCenterModule/MessageCenterModule.h"

namespace kakaIM {

    namespace node {
        class ClusterModule;

        class SessionModule;

        class AuthenticationModule;

        class OnlineStateModule;

        class MessageSendServiceModule;

        class RosterModule;

        class OfflineModule;

        class SingleChatModule;

        class GroupChatModule;

        class MessageCenterModule
                : public net::MessageCenterModule,
                  public ConnectionOperationService,
                  public NodeConnectionQueryService {
        public:
            MessageCenterModule(std::string listenIP, u_int16_t listenPort,
                                std::shared_ptr<net::MessageCenterAdapter> adapter,
                                size_t timeout = 0) : net::MessageCenterModule(listenIP, listenPort, adapter, timeout) {

            }

            virtual bool sendMessageThroughConnection(const std::string connectionIdentifier,
                                                      const ::google::protobuf::Message &message) override {
                return this->sendMessage(connectionIdentifier, message);
            }

            virtual uint64_t queryCurrentNodeConnectionCount() override {
                return this->currentConnectionCount();
            }
        };

        class Node {
        public:
            Node();

            ~Node();

            int start();

            void stop();

            bool init(int argc, char *argv[]);

            std::shared_ptr<ClusterModule> getClusterMoudle() {
                return this->mClusterModulePtr;
            }

        private:
            /**
             * 集群模块
             */
            std::shared_ptr<ClusterModule> mClusterModulePtr;
            /**
             * 客户端消息接收模块
             */
            std::shared_ptr<MessageCenterModule> mMessageCenterModulePtr;
            /**
             * 客户端会话模块
             */
            std::shared_ptr<SessionModule> mSessionModulePtr;
            /**
             * 客户端认证模块
             */
            std::shared_ptr<AuthenticationModule> mAuthenticationModulePtr;
            /**
             * 用户在线模块
             */
            std::shared_ptr<OnlineStateModule> mOnlineStateModulePtr;
            /**
             * 消息发送模块
             */
            std::shared_ptr<MessageSendServiceModule> mMessageSendServiceModulePtr;
            /**
             * 花名册模块
             */
            std::shared_ptr<RosterModule> mRosterModulePtr;
            /**
             * 离线模块
             */
            std::shared_ptr<OfflineModule> mOfflineModulePtr;
            /**
             * 单聊消息转发模块
             */
            std::shared_ptr<SingleChatModule> mSingleChatModulePtr;
            /**
             * 群聊模块
             */
            std::shared_ptr<GroupChatModule> mGroupChatModulePtr;
            sem_t *m_task_semaphore;
        };
    }
}


#endif //KAKAIMCLUSTER_NODE_H
