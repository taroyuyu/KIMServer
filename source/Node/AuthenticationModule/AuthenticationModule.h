//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_AUTHENTICATIONMODULE_H
#define KAKAIMCLUSTER_AUTHENTICATIONMODULE_H

#include <Node/KIMNodeModule/KIMNodeModule.h>
#include <Common/Net/MessageCenterModule/MessageFilter.h>
#include <Common/Net/MessageCenterModule/ConnectionDelegate.h>
#include <Node/Events/UserLogoutEvent.h>
#include <map>
#include <queue>
#include <functional>

namespace kakaIM {
    namespace Node{
        class LoginMessage;
        class RegisterMessage;
    }
    namespace node {
        class AuthenticationModule
                : public KIMNodeModule,
                  public net::MessageFilter,
                  public net::ConnectionDelegate,
                  public QueryUserAccountWithSession,
                  public QueryConnectionWithSession {
        public:
            AuthenticationModule();

            virtual bool
            doFilter(const ::google::protobuf::Message &message, const std::string connectionIdentifier) override;

            virtual void didCloseConnection(const std::string connectionIdentifier) override;

            virtual std::string queryUserAccountWithSession(const std::string &userAccount) override;

            virtual std::string queryConnectionWithSession(const std::string &sessionID) override;
            virtual const std::unordered_set<std::string> messageTypes() override;
        protected:
            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task);
            std::unordered_map<std::string,std::function<void(std::unique_ptr<::google::protobuf::Message>, const std::string)>> mMessageHandlerSet;
        private:
            void
            handleLoginMessage(const kakaIM::Node::LoginMessage &loginMessage, const std::string connectionIdentifier);

            void
            handleRegisterMessage(const kakaIM::Node::RegisterMessage &message, const std::string connectionIdentifier);

            enum class VerifyUserResult {
                DBConnectionNotExit,//数据库连接不存在
                InteralError,//内部错误
                True,//身份真实
                False,//身份错误
            };

            VerifyUserResult verifyUser(const std::string userAccount, const std::string userPassword);

            std::mutex sessionMapMutex;
            /**
             * sessionMap的Key-Value为sessionID-(userAccount,connectionIdentifier)
             */
            std::map<std::string, std::pair<std::string, std::string>> sessionMap;
            /**
             * expireSessionMap的Key-Value为sessionID-(userAccount,connectionIdentifier)
             */
            std::map<std::string, std::pair<std::string, std::string>> expireSessionMap;
        };
    }
}


#endif //KAKAIMCLUSTER_AUTHENTICATIONMODULE_H
