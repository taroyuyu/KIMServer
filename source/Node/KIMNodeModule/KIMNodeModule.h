//
// Created by Kakawater on 2022/10/15.
//

#ifndef KAKAIMCLUSTER_KIMNODEMODULE_H
#define KAKAIMCLUSTER_KIMNODEMODULE_H

#include <Common/KIMModule.h>
#include <Node/Service/ConnectionOperationService.h>
#include <Node/Service/MessageSendService.h>
#include <Node/Service/ClusterService.h>
#include <Node/Service/MessageIDGenerateService.h>
#include <Node/Service/NodeConnectionQueryService.h>
#include <Node/Service/SessionQueryService.h>
#include <Node/Service/LoginDeviceQueryService.h>
#include <Node/Service/MessagePersistenceService.h>
#include <Node/Service/UserRelationService.h>
#include <log4cxx/logger.h>
#include <pqxx/pqxx>

namespace kakaIM {
    namespace node {
        class KIMNodeModule : public common::KIMModule {
        public:
            KIMNodeModule(std::string moduleName);
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
            virtual void execute() override;
            virtual void shouldStop() override;
            std::atomic_bool m_needStop;
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task) = 0;

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
            std::shared_ptr<pqxx::connection> getDBConnection();
        };
    }
}


#endif //KAKAIMCLUSTER_KIMNODEMODULE_H
