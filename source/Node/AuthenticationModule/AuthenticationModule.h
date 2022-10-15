//
// Created by Kakawater on 2018/1/7.
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
#include "../../Common/ConcurrentQueue/ConcurrentLinkedQueue.h"
#include <log4cxx/logger.h>
#include <pqxx/pqxx>
#include <map>
#include <queue>
#include <mutex>
#include <memory>
#include <SimpleAmqpClient/SimpleAmqpClient.h>

namespace kakaIM {
    namespace rpc {
        class AuthenticationRequest;
    }
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

            virtual void start() override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            virtual bool
            doFilter(const ::google::protobuf::Message &message, const std::string connectionIdentifier) override;

            virtual void didCloseConnection(const std::string connectionIdentifier) override;

            virtual std::string queryUserAccountWithSession(const std::string &userAccount) override;

            virtual std::string queryConnectionWithSession(const std::string &sessionID) override;

        protected:
            virtual void execute() override;
            virtual void shouldStop() override;
            std::atomic_bool m_needStop;
        private:
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> messageQueue;

            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> mTaskQueue;

            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task);

            void
            handleLoginMessage(const kakaIM::Node::LoginMessage &loginMessage, const std::string connectionIdentifier);

            void
            handleRegisterMessage(const kakaIM::Node::RegisterMessage &message, const std::string connectionIdentifier);

            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;

            enum VerifyUserResult {
                VerifyUserResult_DBConnectionNotExit,//数据库连接不存在
                VerifyUserResult_InteralError,//内部错误
                VerifyUserResult_True,//身份真实
                VerifyUserResult_False,//身份错误
            };

            VerifyUserResult verifyUser(const std::string userAccount, const std::string userPassword);

            std::mutex mDBConnectionPoolMutex;
            std::queue<std::unique_ptr<pqxx::connection>> mDBConnectionPool;

            std::unique_ptr<pqxx::connection> getDBConnection();

            void releaseDBConnection(std::unique_ptr<pqxx::connection> dbConnection);

            std::mutex sessionMapMutex;
            /**
             * sessionMap的Key-Value为sessionID-(userAccount,connectionIdentifier)
             */
            std::map<std::string, std::pair<std::string, std::string>> sessionMap;
            /**
             * expireSessionMap的Key-Value为sessionID-(userAccount,connectionIdentifier)
             */
            std::map<std::string, std::pair<std::string, std::string>> expireSessionMap;

            std::thread mAuthenticationRPCWorkThread;
            AmqpClient::Channel::ptr_t mAmqpChannel;

            void authenticationRPCListenerWork();

            log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_AUTHENTICATIONMODULE_H
