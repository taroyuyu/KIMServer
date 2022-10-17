//
// Created by Kakawater on 2018/1/7.
//

#include <zconf.h>
#include <Node/AuthenticationModule/AuthenticationModule.h>
#include <Common/EventBus/EventBus.h>
#include <Node/Log/log.h>
#include <Common/proto/KakaIMMessage.pb.h>

namespace kakaIM {
    namespace node {
        AuthenticationModule::AuthenticationModule() : KIMNodeModule(AuthenticationModuleLogger){
        }

        void AuthenticationModule::dispatchMessage(
                std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> &task) {
            auto messageType = task.first->GetTypeName();
            if (messageType == kakaIM::Node::LoginMessage::default_instance().GetTypeName()) {
                handleLoginMessage(*(kakaIM::Node::LoginMessage *) task.first.get(), task.second);
            } else if (messageType == kakaIM::Node::RegisterMessage::default_instance().GetTypeName()) {
                handleRegisterMessage(*(kakaIM::Node::RegisterMessage *) task.first.get(), task.second);
            }
        }

        AuthenticationModule::VerifyUserResult
        AuthenticationModule::verifyUser(const std::string userAccount, const std::string userPassword) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);

            auto dbConnection = std::move(this->getDBConnection());
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << "获取数据库连接失败");
                return VerifyUserResult::DBConnectionNotExit;
            }

            const std::string CheckUserSQLStatement = "CheckUserSQL";
            const std::string checkUserSQL = "SELECT account FROM \"user\" WHERE account = $1 AND password = $2 ;";

            VerifyUserResult verifyResult = VerifyUserResult::InteralError;
            try {
                //开启事务
                pqxx::work dbWork(*dbConnection);

                auto invocation = dbWork.prepared(CheckUserSQLStatement);

                if (!invocation.exists()) {
                    dbConnection->prepare(CheckUserSQLStatement, checkUserSQL);
                }

                //执行
                pqxx::result result = invocation(userAccount)(userPassword).exec();


                //提交
                dbWork.commit();

                if (1 != result.size()) {//登录失败
                    verifyResult = VerifyUserResult::False;
                } else {//登录成功
                    verifyResult = VerifyUserResult::True;
                }


            } catch (const std::exception &e) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " " << e.what());
                verifyResult = VerifyUserResult::InteralError;
            }
            return verifyResult;
        }

        void AuthenticationModule::handleLoginMessage(const kakaIM::Node::LoginMessage &loginMessage,
                                                      const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            kakaIM::Node::ResponseLoginMessage responseLoginMessage;

            responseLoginMessage.set_sessionid(loginMessage.sessionid());

            switch (this->verifyUser(loginMessage.useraccount(), loginMessage.userpassword())) {
                case VerifyUserResult::DBConnectionNotExit:
                case VerifyUserResult::InteralError: {
                    responseLoginMessage.set_loginstate(kakaIM::Node::ResponseLoginMessage::Failed);
                    responseLoginMessage.set_failureerror(
                            kakaIM::Node::ResponseLoginMessage_FailureError_ServerInternalError);
                }
                    break;
                case VerifyUserResult::True: {
                    responseLoginMessage.set_loginstate(kakaIM::Node::ResponseLoginMessage::Success);
                    std::lock_guard<std::mutex> lock(this->sessionMapMutex);
                    auto expireRecordIt = this->expireSessionMap.find(loginMessage.sessionid());
                    if (expireRecordIt != this->expireSessionMap.end()) {
                        this->expireSessionMap.erase(expireRecordIt);
                    }
                    //注册此Session
                    this->sessionMap.emplace(loginMessage.sessionid(),
                                             std::make_pair(loginMessage.useraccount(),
                                                            connectionIdentifier));
                }
                    break;
                case VerifyUserResult::False: {
                    responseLoginMessage.set_loginstate(kakaIM::Node::ResponseLoginMessage::Failed);
                    responseLoginMessage.set_failureerror(
                            kakaIM::Node::ResponseLoginMessage_FailureError_WrongAccountOrPassword);
                }
                    break;
            }

            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseLoginMessage);
            } else {
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " connectionOperationService不存在");
            }
        }

        void AuthenticationModule::handleRegisterMessage(const kakaIM::Node::RegisterMessage &registerMessage,
                                                         const std::string connectionIdentifier) {
            kakaIM::Node::ResponseRegisterMessage responseRegisterMessage;

            auto dbConnection = std::move(this->getDBConnection());
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接失败");
                responseRegisterMessage.set_registerstate(kakaIM::Node::ResponseRegisterMessage::Failed);
                //设置错误原因为用户名已经存在
                responseRegisterMessage.set_failureerror(
                        kakaIM::Node::ResponseRegisterMessage_FailureError_ServerInternalError);
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                             responseRegisterMessage);
                } else {
                    LOG4CXX_ERROR(this->logger, __FUNCTION__ << " connectionOperationService不存在");
                }
                return;;
            }

            const std::string RegisteAccountSQLStatement = "RegisteAccountSQL";
            const std::string UpdateUserVCardSQLStatement = "UpdateUserVCardSQL";
            const std::string registeAccountSQL = "INSERT INTO \"user\" "
                                                  "(account,password,created_at) "
                                                  "VALUES ($1,$2,now());";
            const std::string updateUserVCardSQL = "INSERT INTO user_vcard (account, nickname)"
                                                   " VALUES($1,$2);";
            try {
                //开启事务
                pqxx::work dbWork(*dbConnection);

                auto registeAccountSQLStatementInvocation = dbWork.prepared(RegisteAccountSQLStatement);

                if (!registeAccountSQLStatementInvocation.exists()) {
                    dbConnection->prepare(RegisteAccountSQLStatement, registeAccountSQL);
                }
                //执行
                pqxx::result r = registeAccountSQLStatementInvocation(registerMessage.useraccount())(
                        registerMessage.userpassword()).exec();

                auto updateUserVCardStatementInvocation = dbWork.prepared(UpdateUserVCardSQLStatement);

                if (!updateUserVCardStatementInvocation.exists()) {
                    dbConnection->prepare(UpdateUserVCardSQLStatement, updateUserVCardSQL);
                }

                r = updateUserVCardStatementInvocation(registerMessage.useraccount())(
                        registerMessage.usernickname()).exec();

                //提交
                dbWork.commit();
                responseRegisterMessage.set_registerstate(kakaIM::Node::ResponseRegisterMessage::Success);

            } catch (const std::exception &e) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " 注册用户失败," << e.what());
                std::cerr << e.what() << std::endl;
                responseRegisterMessage.set_registerstate(kakaIM::Node::ResponseRegisterMessage::Failed);
                //设置错误原因为用户名已经存在
                responseRegisterMessage.set_failureerror(
                        kakaIM::Node::ResponseRegisterMessage_FailureError_ServerInternalError);
            }
            //发送Response消息

            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseRegisterMessage);
            } else {
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " connectionOperationService不存在");
            }
        }

        bool AuthenticationModule::doFilter(const ::google::protobuf::Message &message,
                                            const std::string connectionIdentifier) {
            //若是RequestSessionIDMessage、LoginMessage、RegisterMessage、HeartBeatMessage、则直接放行通过
            if (message.GetTypeName() == kakaIM::Node::RequestSessionIDMessage::default_instance().GetTypeName() ||
                message.GetTypeName() == kakaIM::Node::RegisterMessage::default_instance().GetTypeName() ||
                message.GetTypeName() == kakaIM::Node::HeartBeatMessage::default_instance().GetTypeName() ||
                message.GetTypeName() == kakaIM::Node::LoginMessage::default_instance().GetTypeName()) {
                return true;
            } else {//其余消息
                //1.获取sessionID
                std::string sessionID = message.GetReflection()->GetString(message,
                                                                           message.GetDescriptor()->FindFieldByName(
                                                                                   "sessionID"));
                std::lock_guard<std::mutex> lock(this->sessionMapMutex);
                //2.检验sessionID对应的connection是否与发送此消息的connection是否相同
                auto pairIt = this->sessionMap.find(sessionID);
                //不存在此sessionID
                if (pairIt == this->sessionMap.end()) {
                    return false;
                } else {
                    //连接sessionID存在
                    if (pairIt->second.second == connectionIdentifier) {//是同一连接
                        return true;
                    } else {//不是同一连接
                        return false;
                    }
                }
            }
        }

        void AuthenticationModule::didCloseConnection(const std::string connectionIdentifier) {
            std::lock_guard<std::mutex> lock(this->sessionMapMutex);
            auto connectionPairIt = std::find_if(this->sessionMap.begin(), this->sessionMap.end(),
                                                 [connectionIdentifier](
                                                         const std::map<std::string, std::pair<std::string, std::string>>::value_type &item) -> bool {
                                                     return item.second.second == connectionIdentifier;
                                                 });
            if (connectionPairIt != this->sessionMap.end()) {
                std::string sessionID = connectionPairIt->first;
                std::string userAccount = connectionPairIt->second.first;
                this->sessionMap.erase(connectionPairIt);
                auto recordIt = this->expireSessionMap.find(sessionID);
                if (recordIt != this->expireSessionMap.end()) {
                    this->expireSessionMap.erase(recordIt);
                }
                this->expireSessionMap.emplace(sessionID, std::make_pair(userAccount, connectionIdentifier));
                std::shared_ptr<UserLogoutEvent> event(new UserLogoutEvent(sessionID, userAccount));
                kakaIM::EventBus::getDefault().Post(event);
            }
        }

        std::string AuthenticationModule::queryUserAccountWithSession(const std::string &sessionID) {
            std::lock_guard<std::mutex> lock(this->sessionMapMutex);
            auto pairIt = this->sessionMap.find(sessionID);
            if (pairIt == this->sessionMap.end()) {
                auto expireRecordIt = this->expireSessionMap.find(sessionID);
                if (expireRecordIt != this->expireSessionMap.end()) {
                    return expireRecordIt->second.first;
                } else {
                    return "";
                }
            } else {
                return pairIt->second.first;
            }
        }

        std::string AuthenticationModule::queryConnectionWithSession(const std::string &sessionID) {
            std::lock_guard<std::mutex> lock(this->sessionMapMutex);
            auto pairIt = this->sessionMap.find(sessionID);
            if (pairIt == this->sessionMap.end()) {
                auto expireRecordIt = this->expireSessionMap.find(sessionID);
                if (expireRecordIt != this->expireSessionMap.end()) {
                    return expireRecordIt->second.second;
                } else {
                    return "";
                }
            } else {
                return pairIt->second.second;
            }
        }
    }
}
