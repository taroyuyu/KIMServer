//
// Created by 凌宇 on 2022/10/15.
//

#include "KIMNodeModule.h"

namespace kakaIM {
    namespace node {
        KIMNodeModule::KIMNodeModule(std::string moduleName) : logger(log4cxx::Logger::getLogger(moduleName)),m_needStop(false) {
        }

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

        void KIMNodeModule::setMessagePersistenceService(std::weak_ptr<MessagePersistenceService> service) {
            this->mMessagePersistenceServicePtr = service;
        }

        void KIMNodeModule::setUserRelationService(std::weak_ptr<UserRelationService> userRelationServicePtr) {
            this->mUserRelationServicePtr = userRelationServicePtr;
        }

        std::shared_ptr<pqxx::connection> KIMNodeModule::getDBConnection() {
            if (this->m_dbConnection) {
                return this->m_dbConnection;
            }

            const std::string postgrelConnectionUrl =
                    "dbname=" + this->dbConfig.getDBName() + " user=" + this->dbConfig.getUserAccount() + " password=" +
                    this->dbConfig.getUserPassword() + " hostaddr=" + this->dbConfig.getHostAddr() + " port=" +
                    std::to_string(this->dbConfig.getPort());

            try {

                this->m_dbConnection = std::make_shared<pqxx::connection>(postgrelConnectionUrl);

                if (!this->m_dbConnection->is_open()) {
                    LOG4CXX_FATAL(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "打开数据库失败");
                }


            } catch (const std::exception &exception) {
                LOG4CXX_FATAL(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << "连接数据库异常," << exception.what());
            }

            return this->m_dbConnection;
        }

        void KIMNodeModule::execute() {
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (not this->m_needStop) {
                if (auto task = this->mTaskQueue.try_pop()) {
                    this->dispatchMessage(*task);
                } else {
                    std::this_thread::yield();
                }
            }

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }
    }
}