//
// Created by taroyuyu on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_SINGLECHATMODULE_H
#define KAKAIMCLUSTER_SINGLECHATMODULE_H

#include <queue>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../Service/MessageSendService.h"
#include "../Service/ClusterService.h"
#include "../Service/MessageIDGenerateService.h"
#include "../Service/LoginDeviceQueryService.h"
#include "../Service/SessionQueryService.h"
#include "../Service/MessagePersistenceService.h"
#include "../Service/UserRelationService.h"
#include "../Service/ConnectionOperationService.h"

namespace kakaIM {
    namespace node {
        class SingleChatModule : public common::KIMModule{
        public:
            SingleChatModule();

            ~SingleChatModule();

            virtual bool init() override;

            void addChatMessage(const kakaIM::Node::ChatMessage &message, const std::string connectionIdentifier);

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void setMessageSendService(std::weak_ptr<MessageSendService> messageSendServicePtr);

            void setMessageIDGenerateService(std::weak_ptr<MessageIDGenerateService> service);

            void setClusterService(std::weak_ptr<ClusterService> service);

            void setLoginDeviceQueryService(std::weak_ptr<LoginDeviceQueryService> service);

            void setQueryUserAccountWithSessionService(std::weak_ptr<QueryUserAccountWithSession> service);

            void setQueryConnectionWithSessionService(std::weak_ptr<QueryConnectionWithSession> service);

            void setMessagePersistenceService(std::weak_ptr<MessagePersistenceService> service);

            void setUserRelationService(std::weak_ptr<UserRelationService> service);

        protected:
            virtual void execute() override;

        protected:
            int mEpollInstance;
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> messageQueue;

            void handleChatMessage(kakaIM::Node::ChatMessage &chatMessage, const std::string connectionIdentifier);


            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<MessageSendService> mMessageSendServicePtr;
            std::weak_ptr<ClusterService> mClusterServicePtr;
            std::weak_ptr<LoginDeviceQueryService> mLoginDeviceQueryServicePtr;
            std::weak_ptr<QueryUserAccountWithSession> mQueryUserAccountWithSessionServicePtr;
            std::weak_ptr<QueryConnectionWithSession> mQueryConnectionWithSessionServicePtr;
            std::weak_ptr<MessageIDGenerateService> mMessageIDGenerateServicePtr;
            std::weak_ptr<MessagePersistenceService> mMessagePersistenceServicePtr;
            std::weak_ptr<UserRelationService> mUserRelationServicePtr;
            log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_SINGLECHATMODULE_H

