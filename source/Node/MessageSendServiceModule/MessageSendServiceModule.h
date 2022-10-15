//
// Created by Kakawater on 2018/1/26.
//

#ifndef KAKAIMCLUSTER_MESSAGESENDSERVICEMODULE_H
#define KAKAIMCLUSTER_MESSAGESENDSERVICEMODULE_H

#include <google/protobuf/message.h>
#include <queue>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../Service/MessageSendService.h"
#include "../Service/MessageIDGenerateService.h"
#include "../Service/LoginDeviceQueryService.h"
#include "../Service/SessionQueryService.h"
#include "../Service/ClusterService.h"
#include "../Service/ConnectionOperationService.h"

namespace kakaIM {
    namespace node {
        class MessageSendServiceModule
                : public common::KIMModule, public MessageSendService, public ClusterService::ServerMessageListener {
        public:
            MessageSendServiceModule();

            ~MessageSendServiceModule();

            virtual bool init() override;

            /**
             * 发送消息给特定的用户
             * @param userAccount 用户账号
             * @param message 待发送的消息
             * @return
             */
            virtual void
            sendMessageToUser(const std::string &userAccount, const ::google::protobuf::Message &message) override;

            virtual void
            sendMessageToSession(const std::string &serverID,const std::string & sessionID, const ::google::protobuf::Message &message)override ;
            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);


            void setLoginDeviceQueryService(std::weak_ptr<LoginDeviceQueryService> service);

            void setQueryConnectionWithSessionService(std::weak_ptr<QueryConnectionWithSession> service);

            void setClusterService(std::weak_ptr<ClusterService> service);

            virtual void
            didReceivedServerMessageFromCluster(const kakaIM::president::ServerMessage &serverMessage) override;

        protected:
            virtual void execute() override;

        private:
            int mEpollInstance;
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<MessageIDGenerateService> mMessageIDGenerateServicePtr;
            std::weak_ptr<LoginDeviceQueryService> mLoginDeviceQueryServicePtr;
            std::weak_ptr<QueryConnectionWithSession> mQueryConnectionWithSessionServicePtr;
            std::weak_ptr<ClusterService> mClusterServicePtr;
            int messageEventfd;
            std::mutex mMessageQueueMutex;
            std::queue<std::pair<const std::string, std::unique_ptr<::google::protobuf::Message>>> mMessageQueue;

            void handleMessageForSend(const std::string &userAccount, ::google::protobuf::Message &message);

	    void handleSessionMessage(kakaIM::president::SessionMessage & message);

            int serverMessageEventfd;
            std::mutex mServerMessageQueueMutex;
            std::queue<std::unique_ptr<kakaIM::president::ServerMessage>> mServerMessageQueue;
            void handleServerMessageFromCluster(const kakaIM::president::ServerMessage & serverMessage);
            log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_MESSAGESENDSERVICEMODULE_H
