//
// Created by taroyuyu on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_AUTHENTICATIONMODULE_H
#define KAKAIMCLUSTER_AUTHENTICATIONMODULE_H

#include "../../Common/KIMModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../../Common/Net/MessageCenterModule/MessageFilter.h"
#include "../../Common/Net/MessageCenterModule/ConnectionDelegate.h"
#include "../Events/UserLogoutEvent.h"
#include "../Service/SessionQueryService.h"
#include "../Service/ConnectionOperationService.h"
#include "../../Common/KIMDBConfig.h"
#include <log4cxx/logger.h>
#include <pqxx/pqxx>
#include <map>
#include <queue>
#include <mutex>
#include <memory>

namespace kakaIM {
    namespace node {
        class AuthenticationModule
                : public common::KIMModule,
                  public net::MessageFilter,
                  public net::ConnectionDelegate,
                  public QueryUserAccountWithSession,
                  public QueryConnectionWithSession {
        public:
            AuthenticationModule();

            ~AuthenticationModule();

            virtual bool init() override;

            void setDBConfig(const common::KIMDBConfig &dbConfig);

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void addLoginMessage(const kakaIM::Node::LoginMessage &message, const std::string connectionIdentifier);

            void
            addRegisterMessage(const kakaIM::Node::RegisterMessage &message, const std::string connectionIdentifier);

            virtual bool
            doFilter(const ::google::protobuf::Message &message, const std::string connectionIdentifier) override;

            virtual void didCloseConnection(const std::string connectionIdentifier) override;

            virtual std::string queryUserAccountWithSession(const std::string &userAccount) override;

            virtual std::string queryConnectionWithSession(const std::string &sessionID) override;

        protected:
            virtual void execute() override;

        private:
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> messageQueue;

            void
            handleLoginMessage(const kakaIM::Node::LoginMessage &loginMessage, const std::string connectionIdentifier);

            void
            handleRegisterMessage(const kakaIM::Node::RegisterMessage &message, const std::string connectionIdentifier);

            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;

            /**
             * @description 数据库连接
             */
            common::KIMDBConfig dbConfig;
            std::shared_ptr<pqxx::connection> m_dbConnection;

            std::shared_ptr<pqxx::connection> getDBConnection();

            std::mutex sessionMapMutex;
            /**
             * sessionMap的Key-Value为sessionID-(userAccount,connectionIdentifier)
             */
            std::map<std::string, std::pair<std::string, std::string>> sessionMap;
            /**
             * expireSessionMap的Key-Value为sessionID-(userAccount,connectionIdentifier)
             */
            std::map<std::string,std::pair<std::string, std::string>> expireSessionMap;
            log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_AUTHENTICATIONMODULE_H
