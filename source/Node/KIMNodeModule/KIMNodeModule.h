//
// Created by Kakawater on 2022/10/15.
//

#ifndef KAKAIMCLUSTER_KIMNODEMODULE_H
#define KAKAIMCLUSTER_KIMNODEMODULE_H

#include "../../Common/KIMModule.h"
#include "../Service/ConnectionOperationService.h"
#include "../Service/MessageSendService.h"
#include "../Service/ClusterService.h"
#include "../Service/MessageIDGenerateService.h"
#include "../Service/NodeConnectionQueryService.h"
#include "../Service/SessionQueryService.h"
#include "../Service/LoginDeviceQueryService.h"
#include "../Service/MessagePersistenceService.h"
#include "../Service/UserRelationService.h"
#include <memory>
#include <log4cxx/logger.h>
#include <pqxx/pqxx>

namespace kakaIM {
    namespace node {
        class KIMNodeModule : public common::KIMModule {
        public:
            virtual void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);
            virtual void setMessageSendService(std::weak_ptr<MessageSendService> messageSendServicePtr);
            virtual void setClusterService(std::weak_ptr<ClusterService> service);
            virtual void setMessageIDGenerateService(std::weak_ptr<MessageIDGenerateService> service);;
            virtual void setNodeConnectionQueryService(std::weak_ptr<NodeConnectionQueryService> nodeConnectionQueryServicePtr);
            virtual void setQueryUserAccountWithSessionService(std::weak_ptr<QueryUserAccountWithSession> queryUserAccountWithSessionServicePtr);
            virtual void setQueryConnectionWithSessionService(std::weak_ptr<QueryConnectionWithSession> service);
            virtual void setLoginDeviceQueryService(std::weak_ptr<LoginDeviceQueryService> service);
            virtual void setMessagePersistenceService(std::weak_ptr<MessagePersistenceService> service);
            virtual void setUserRelationService(std::weak_ptr<UserRelationService> userRelationServicePtr);
        protected:
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<MessageSendService> mMessageSendServicePtr;
            std::weak_ptr<ClusterService> mClusterServicePtr;
            std::weak_ptr<MessageIDGenerateService> mMessageIDGenerateServicePtr;
            std::weak_ptr<NodeConnectionQueryService> nodeConnectionQueryServicePtr;
            std::weak_ptr<QueryUserAccountWithSession> mQueryUserAccountWithSessionServicePtr;
            std::weak_ptr<QueryConnectionWithSession> mQueryConnectionWithSessionServicePtr;
            std::weak_ptr<LoginDeviceQueryService> mLoginDeviceQueryServicePtr;
            std::weak_ptr<MessagePersistenceService> mMessagePersistenceServicePtr;
            std::weak_ptr<UserRelationService> mUserRelationServicePtr;
            log4cxx::LoggerPtr logger;
            std::shared_ptr<pqxx::connection> m_dbConnection;

        };
    }
}


#endif //KAKAIMCLUSTER_KIMNODEMODULE_H
