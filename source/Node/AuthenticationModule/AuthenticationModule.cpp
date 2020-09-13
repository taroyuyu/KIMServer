//
// Created by taroyuyu on 2018/1/7.
//

#include <zconf.h>
#include "AuthenticationModule.h"
#include <sys/eventfd.h>
#include "../../Common/EventBus/EventBus.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace node {
        AuthenticationModule::AuthenticationModule() : messageEventfd(-1), m_dbConnection(nullptr) {
            this->logger = log4cxx::Logger::getLogger(AuthenticationModuleLogger);
        }

        AuthenticationModule::~AuthenticationModule() {
            while (false == this->messageQueue.empty()) {
                this->messageQueue.pop();
            }
            if (this->m_dbConnection && this->m_dbConnection->is_open()) {
                this->m_dbConnection->disconnect();
            }
        }

        bool AuthenticationModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }
            return true;
        }

        void AuthenticationModule::setDBConfig(const common::KIMDBConfig &dbConfig) {
            this->dbConfig = dbConfig;
        }

        void AuthenticationModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void AuthenticationModule::execute() {
            while (this->m_isStarted) {
                uint64_t count;
                if (0 < read(this->messageEventfd, &count, sizeof(count))) {
                    while (count-- && false == this->messageQueue.empty()) {
                        this->messageQueueMutex.lock();
                        auto pairIt = std::move(this->messageQueue.front());
                        this->messageQueue.pop();
                        this->messageQueueMutex.unlock();

                        auto messageType = pairIt.first->GetTypeName();
                        if (messageType == kakaIM::Node::LoginMessage::default_instance().GetTypeName()) {
                            handleLoginMessage(*(kakaIM::Node::LoginMessage *) pairIt.first.get(), pairIt.second);
                        } else if (messageType == kakaIM::Node::RegisterMessage::default_instance().GetTypeName()) {
                            handleRegisterMessage(*(kakaIM::Node::RegisterMessage *) pairIt.first.get(), pairIt.second);
                        }
                    }

                } else {
                    LOG4CXX_WARN(this->logger,
                                 typeid(this).name() << "" << __FUNCTION__ << "read(messageEventfd)操作出错，errno ="
                                                     << errno);
                }
            }
        }

        void
        AuthenticationModule::addLoginMessage(const kakaIM::Node::LoginMessage &message,
                                              const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::LoginMessage> loginMessage(new kakaIM::Node::LoginMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(loginMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void AuthenticationModule::addRegisterMessage(const kakaIM::Node::RegisterMessage &message,
                                                      const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::RegisterMessage> registerMessage(new kakaIM::Node::RegisterMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(registerMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void AuthenticationModule::handleLoginMessage(const kakaIM::Node::LoginMessage &loginMessage,
                                                      const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            kakaIM::Node::ResponseLoginMessage responseLoginMessage;

            responseLoginMessage.set_sessionid(loginMessage.sessionid());

            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger,__FUNCTION__ << "获取数据库连接失败");
                responseLoginMessage.set_loginstate(Node::ResponseLoginMessage_LoginState_Failed);
                responseLoginMessage.set_failureerror(Node::ResponseLoginMessage_FailureError_ServerInternalError);
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseLoginMessage);
                }else{
                    LOG4CXX_ERROR(this->logger,__FUNCTION__ << " connectionOperationService不存在");
                }
                return;;
            }

            const std::string CheckUserSQLStatement = "CheckUserSQL";
            const std::string checkUserSQL = "SELECT account FROM \"user\" WHERE account = $1 AND password = $2 ;";

            try {
                //开启事务
                pqxx::work dbWork(*dbConnection);

                auto invocation = dbWork.prepared(CheckUserSQLStatement);

                if (!invocation.exists()) {
                    dbConnection->prepare(CheckUserSQLStatement, checkUserSQL);
                }

                //执行
                pqxx::result result = invocation(loginMessage.useraccount())(loginMessage.userpassword()).exec();

                if (1 != result.size()) {//登录失败
                    responseLoginMessage.set_loginstate(kakaIM::Node::ResponseLoginMessage::Failed);
                    responseLoginMessage.set_failureerror(
                            kakaIM::Node::ResponseLoginMessage_FailureError_WrongAccountOrPassword);
                } else {//登录成功
                    responseLoginMessage.set_loginstate(kakaIM::Node::ResponseLoginMessage::Success);
                    std::lock_guard<std::mutex> lock(this->sessionMapMutex);
                    auto expireRecordIt = this->expireSessionMap.find(loginMessage.sessionid());
                    if(expireRecordIt != this->expireSessionMap.end()){
                        this->expireSessionMap.erase(expireRecordIt);
                    }
                    //注册此Session
                    this->sessionMap.emplace(loginMessage.sessionid(),
                                                           std::make_pair(loginMessage.useraccount(),
                                                                          connectionIdentifier));

                }

                //提交
                dbWork.commit();


            } catch (const std::exception &e) {
                std::cerr << e.what() << std::endl;
                responseLoginMessage.set_loginstate(kakaIM::Node::ResponseLoginMessage::Failed);
                //服务器内部错误
                responseLoginMessage.set_failureerror(kakaIM::Node::ResponseLoginMessage_FailureError_ServerInternalError);
            }

            //发送Response消息
            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseLoginMessage);
            }else{
                LOG4CXX_ERROR(this->logger,__FUNCTION__ << " connectionOperationService不存在");
            }
        }

        void AuthenticationModule::handleRegisterMessage(const kakaIM::Node::RegisterMessage &registerMessage,
                                                         const std::string connectionIdentifier) {
            kakaIM::Node::ResponseRegisterMessage responseRegisterMessage;

            responseRegisterMessage.set_sessionid(registerMessage.sessionid());

            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接失败");
                responseRegisterMessage.set_registerstate(kakaIM::Node::ResponseRegisterMessage::Failed);
                //设置错误原因为用户名已经存在
                responseRegisterMessage.set_failureerror(
                        kakaIM::Node::ResponseRegisterMessage_FailureError_ServerInternalError);
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseRegisterMessage);
                }else{
                    LOG4CXX_ERROR(this->logger,__FUNCTION__ << " connectionOperationService不存在");
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
                LOG4CXX_ERROR(this->logger,__FUNCTION__<<" 注册用户失败,"<<e.what());
                std::cerr << e.what() << std::endl;
                responseRegisterMessage.set_registerstate(kakaIM::Node::ResponseRegisterMessage::Failed);
                //设置错误原因为用户名已经存在
                responseRegisterMessage.set_failureerror(
                        kakaIM::Node::ResponseRegisterMessage_FailureError_ServerInternalError);
            }
            //发送Response消息

            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseRegisterMessage);
            }else{
                LOG4CXX_ERROR(this->logger,__FUNCTION__ << " connectionOperationService不存在");
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
                if(recordIt != this->expireSessionMap.end()){
                    this->expireSessionMap.erase(recordIt);
                }
                this->expireSessionMap.emplace(sessionID,std::make_pair(userAccount,connectionIdentifier));
                std::shared_ptr<UserLogoutEvent> event(new UserLogoutEvent(sessionID, userAccount));
                kakaIM::EventBus::getDefault().Post(event);
            }
        }

        std::string AuthenticationModule::queryUserAccountWithSession(const std::string &sessionID)  {
            std::lock_guard<std::mutex> lock(this->sessionMapMutex);
            auto pairIt = this->sessionMap.find(sessionID);
            if (pairIt == this->sessionMap.end()) {
                auto expireRecordIt = this->expireSessionMap.find(sessionID);
                if(expireRecordIt != this->expireSessionMap.end()){
                    return expireRecordIt->second.first;
                }else{
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
                if(expireRecordIt != this->expireSessionMap.end()){
                    return expireRecordIt->second.second;
                }else{
                    return "";
                }
            } else {
                return pairIt->second.second;
            }
        }

        std::shared_ptr<pqxx::connection> AuthenticationModule::getDBConnection() {

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
                    LOG4CXX_FATAL(this->logger, typeid(this).name() << "" << __FUNCTION__ << "打开数据库失败");
                }


            } catch (const std::exception &exception) {

                LOG4CXX_FATAL(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "打开数据库失败," << exception.what());
            }

            return this->m_dbConnection;

        }
    }
}
