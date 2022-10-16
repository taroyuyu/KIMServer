//
// Created by 凌宇 on 2022/10/15.
//

#include "KIMNodeModule.h"

namespace kakaIM {
    namespace node {
        void KIMNodeModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void KIMNodeModule::setMessageSendService(std::weak_ptr<MessageSendService> messageSendServicePtr) {
            this->mMessageSendServicePtr = messageSendServicePtr;
        }

        void KIMNodeModule::setClusterService(std::weak_ptr<ClusterService> service) {
            this->mClusterServicePtr = service;
        }

        void KIMNodeModule::setMessageIDGenerateService(std::weak_ptr<MessageIDGenerateService> service) {
            this->mMessageIDGenerateServicePtr = service;
        }

        void KIMNodeModule::setNodeConnectionQueryService(
                std::weak_ptr<NodeConnectionQueryService> nodeConnectionQueryServicePtr) {
            this->nodeConnectionQueryServicePtr = nodeConnectionQueryServicePtr;
        }

        void KIMNodeModule::setQueryUserAccountWithSessionService(
                std::weak_ptr<QueryUserAccountWithSession> queryUserAccountWithSessionServicePtr) {
            this->mQueryUserAccountWithSessionServicePtr = queryUserAccountWithSessionServicePtr;
        }

        void KIMNodeModule::setQueryConnectionWithSessionService(std::weak_ptr<QueryConnectionWithSession> service) {
            this->mQueryConnectionWithSessionServicePtr = service;
        }

        void KIMNodeModule::setLoginDeviceQueryService(std::weak_ptr<LoginDeviceQueryService> service) {
            this->mLoginDeviceQueryServicePtr = service;
        }
    }
}