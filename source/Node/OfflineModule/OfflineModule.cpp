//
// Created by Kakawater on 2018/1/7.
//

#include <zconf.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "OfflineModule.h"
#include "../../Common/util/Date.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace node {
        OfflineModule::OfflineModule() : mEpollInstance(-1), messageEventfd(-1), persistTaskQueueEventfd(-1),
                                         m_dbConnection(nullptr) {
            this->logger = log4cxx::Logger::getLogger(OfflineModuleLogger);
        }

        OfflineModule::~OfflineModule() {
            while (false == this->messageQueue.empty()) {
                this->messageQueue.pop();
            }
            while (!this->persistTaskQueue.empty()) {
                this->persistTaskQueue.pop();
            }
            if (this->m_dbConnection && this->m_dbConnection->is_open()) {
                this->m_dbConnection->disconnect();
            }
        }

        bool OfflineModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }
            this->persistTaskQueueEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->persistTaskQueueEventfd < 0) {
                return false;
            }

            //创建Epoll实例
            if (-1 == (this->mEpollInstance = epoll_create1(0))) {
                return false;
            }

            //向Epill实例注册messageEventfd,和clusterEventfd
            struct epoll_event messageEventfdEvent;
            messageEventfdEvent.events = EPOLLIN;
            messageEventfdEvent.data.fd = this->messageEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->messageEventfd, &messageEventfdEvent)) {
                return false;
            }
            struct epoll_event taskQueuefdEvent;
            taskQueuefdEvent.events = EPOLLIN;
            taskQueuefdEvent.data.fd = this->persistTaskQueueEventfd;
            if (-1 ==
                epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->persistTaskQueueEventfd, &taskQueuefdEvent)) {
                return false;
            }
            return true;
        }

        void OfflineModule::setDBConfig(const common::KIMDBConfig &dbConfig) {
            this->dbConfig = dbConfig;
        }

        void OfflineModule::execute() {
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (this->m_isStarted) {
                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   1000);

                if (-1 == happedEventsCount) {
                    LOG4CXX_WARN(this->logger,
                                 typeid(this).name() << "" << __FUNCTION__ << " 等待Epill实例上的事件出错，errno ="
                                                     << errno);
                }
                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (EPOLLIN & happedEvents[i].events) {
                        if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < ::read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->messageQueue.empty()) {
                                    this->messageQueueMutex.lock();
                                    auto pairIt = std::move(this->messageQueue.front());
                                    this->messageQueue.pop();
                                    this->messageQueueMutex.unlock();

                                    auto messageType = pairIt.first->GetTypeName();
                                    if (messageType ==
                                        kakaIM::Node::PullChatMessage::default_instance().GetTypeName()) {
                                        this->handlePullChatMessage(
                                                *(kakaIM::Node::PullChatMessage *) pairIt.first.get(),
                                                pairIt.second);
                                    } else if (messageType ==
                                               kakaIM::Node::PullGroupChatMessage::default_instance().GetTypeName()) {
                                        this->handlePullGroupChatMessage(
                                                *(kakaIM::Node::PullGroupChatMessage *) pairIt.first.get(),
                                                pairIt.second);
                                    }
                                }

                            } else {
                                LOG4CXX_WARN(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                               << "read(messageEventfd)操作出错，errno ="
                                                                               << errno);
                            }
                        } else if (this->persistTaskQueueEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < ::read(this->persistTaskQueueEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->persistTaskQueue.empty()) {
                                    this->persistTaskQueueMutex.lock();
                                    auto task = std::move(this->persistTaskQueue.front());
                                    this->persistTaskQueue.pop();
                                    this->persistTaskQueueMutex.unlock();

                                    if (0 == strcmp(typeid(*task).name(), (typeid(ChatMessagePersistTask).name()))) {
                                        ChatMessagePersistTask *chatMessagePersistTask = reinterpret_cast<ChatMessagePersistTask *>(task.get());
                                        this->handleChatMessagePersist(chatMessagePersistTask->getUserAccount(),
                                                                       chatMessagePersistTask->getMessage(),
                                                                       chatMessagePersistTask->getMessageID());
                                    } else if (0 == strcmp(typeid(*task).name(),
                                                           typeid(GroupChatMessagePersistTask).name())) {
                                        GroupChatMessagePersistTask *groupChatMessagePersistTask = reinterpret_cast<GroupChatMessagePersistTask *>(task.get());
                                        this->handletGroupChatMessagePersis(groupChatMessagePersistTask->getGroupId(),
                                                                            groupChatMessagePersistTask->getMessage(),
                                                                            groupChatMessagePersistTask->getMessageID());
                                    }
                                }
                            }
                        }
                    }
                }
            }

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }

        void OfflineModule::shouldStop() {
            this->m_needStop = true;
        }

        void OfflineModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void OfflineModule::setQueryUserAccountWithSessionService(
                std::weak_ptr<QueryUserAccountWithSession> queryUserAccountWithSessionServicePtr) {
            this->mQueryUserAccountWithSessionServicePtr = queryUserAccountWithSessionServicePtr;
        }

        void OfflineModule::addPullChatMessage(std::unique_ptr<kakaIM::Node::PullChatMessage> message,
                                               const std::string connectionIdentifier) {
            if (!message) {
                return;
            }
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::make_pair(std::move(message), connectionIdentifier));
            const uint64_t count = 1;
            //增加信号量
            if (8 != ::write(this->messageEventfd, &count, sizeof(count))) {
                LOG4CXX_WARN(this->logger,
                             typeid(this).name() << "" << __FUNCTION__ << " 增加信号量失败，errno =" << errno);
            }
        }

        void OfflineModule::addPullGroupChatMessage(std::unique_ptr<kakaIM::Node::PullGroupChatMessage> message,
                                                    const std::string connectionIdentifier) {
            if (!message) {
                return;
            }
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::make_pair(std::move(message), connectionIdentifier));
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }


        void OfflineModule::persistChatMessage(std::string userAccount, const kakaIM::Node::ChatMessage &message,
                                               const uint64_t messageID) {
            std::unique_ptr<ChatMessagePersistTask> task(new ChatMessagePersistTask(userAccount, message, messageID));
            std::lock_guard<std::mutex> lock(this->persistTaskQueueMutex);
            this->persistTaskQueue.emplace(std::move(task));
            uint64_t count = 1;
            //增加信号量
            ::write(this->persistTaskQueueEventfd, &count, sizeof(count));
        }

        void OfflineModule::persistGroupChatMessage(const kakaIM::Node::GroupChatMessage &groupChatMessage,
                                                    const uint64_t messageID) {
            static size_t messageCount = 0;
            LOG4CXX_TRACE(this->logger, __FUNCTION__ << " messageCount=" << ++messageCount);
            std::unique_ptr<GroupChatMessagePersistTask> task(
                    new GroupChatMessagePersistTask(groupChatMessage.groupid(), groupChatMessage, messageID));
            std::lock_guard<std::mutex> lock(this->persistTaskQueueMutex);
            this->persistTaskQueue.emplace(std::move(task));
            uint64_t count = 1;
            //增加信号量
            ::write(this->persistTaskQueueEventfd, &count, sizeof(count));
        }

        void OfflineModule::handleChatMessagePersist(std::string userAccount, const kakaIM::Node::ChatMessage &message,
                                                     const uint64_t messageID) {

            auto dbConnection = this->getDBConnection();

            if (!dbConnection) {
                //异常处理
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 获取数据库连接失败");
                return;
            }

            static const std::string ChatMessagePersistSQLStatement = "ChatMessagePersistSQL";
            static const std::string chatMessagePersistSQL = "INSERT INTO single_chat_timeline "
                                                             "(owner, msg_id, msg_sender, msg_receiver, msg_content,msg_timestamp) "
                                                             "VALUES "
                                                             "($1,$2,$3,$4,$5,$6);";
            try {
                pqxx::work dbWork(*dbConnection);
                auto invocation = dbWork.prepared(ChatMessagePersistSQLStatement);

                if (!invocation.exists()) {
                    dbConnection->prepare(ChatMessagePersistSQLStatement, chatMessagePersistSQL);
                }

                pqxx::result result = invocation(userAccount)(messageID)(message.senderaccount())(
                        message.receiveraccount())(message.content())(message.timestamp()).exec();

                //提交事务
                dbWork.commit();
                if (1 != result.affected_rows()) {//保存失败
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "保存群聊数据失败");
                }
            } catch (pqxx::sql_error exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "执行出错," << exception.what());

            }

        }

        void OfflineModule::handletGroupChatMessagePersis(const std::string groupId,
                                                          const kakaIM::Node::GroupChatMessage &message,
                                                          const uint64_t messageID) {
            static size_t messageCount = 0;
            static size_t messagehandleErrorCount = 0;
            LOG4CXX_TRACE(this->logger, __FUNCTION__ << " messageCount=" << ++messageCount);
            auto dbConnection = this->getDBConnection();

            if (!dbConnection) {
                //异常处理
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接失败");
                return;
            }

            const std::string GroupChatMessagePersistSQLStatement = "GroupChatMessagePersistSQL";
            const std::string groupChatMessagePersistSQL = "INSERT INTO group_chat_timeline "
                                                           "(group_id, msg_sender, msg_content, msg_id) "
                                                           "VALUES ($1,$2,$3,$4);";
            try {
                pqxx::work dbWork(*dbConnection);
                auto invocation = dbWork.prepared(GroupChatMessagePersistSQLStatement);

                if (!invocation.exists()) {
                    dbConnection->prepare(GroupChatMessagePersistSQLStatement, groupChatMessagePersistSQL);
                }

                pqxx::result result = invocation(groupId)(message.sender())(message.content())(messageID).exec();

                //提交事务
                dbWork.commit();
                if (1 != result.affected_rows()) {//保存失败
                    LOG4CXX_ERROR(this->logger,
                                  __FUNCTION__ << " 保存群聊消息失败 messageCount=" << ++messagehandleErrorCount);
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "保存群聊消息失败");
                }
            } catch (pqxx::sql_error exception) {
                LOG4CXX_ERROR(this->logger,
                              __FUNCTION__ << " 保存群聊消息失败 messageCount=" << ++messagehandleErrorCount);
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "保存群聊消息失败," << exception.what());
            }
        }

        void OfflineModule::handlePullChatMessage(const kakaIM::Node::PullChatMessage &pullOfflineMessage,
                                                  const std::string connectionIdentifier) {
            //1.获取用户账号和消息ID
            std::string sessionID = pullOfflineMessage.sessionid();
            uint64_t messageID = pullOfflineMessage.messageid();
            auto queryUserAccountWithSessionService = this->mQueryUserAccountWithSessionServicePtr.lock();
            if (!queryUserAccountWithSessionService) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                << "无法处理，由于不存在queryUserAccountWithSessionService");
                return;
            }
            auto userAccount = queryUserAccountWithSessionService->queryUserAccountWithSession(sessionID);
            std::list<Node::ChatMessage> chatMessageList;
            const std::string QueryUserOfflineChatMessageSQLStatement = "QueryUserOfflineChatMessageSQL";
            const std::string queryUserOfflineChatMessageSQL = "SELECT msg_id,msg_sender,msg_receiver,msg_content,msg_timestamp FROM single_chat_timeline WHERE owner = $1 AND msg_id > $2 ;";
            //2.打开数据，将消息存入数据库

            auto dbConnection = this->getDBConnection();

            if (!dbConnection) {
                //异常处理
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接失败");
                return;
            }

            try {

                pqxx::work dbWork(*dbConnection);

                auto invocation = dbWork.prepared(QueryUserOfflineChatMessageSQLStatement);

                if (!invocation.exists()) {
                    dbConnection->prepare(QueryUserOfflineChatMessageSQLStatement, queryUserOfflineChatMessageSQL);
                }

                pqxx::result result = invocation(userAccount)(messageID).exec();
                for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                    kakaIM::Node::ChatMessage chatMessage;
                    chatMessage.set_messageid(row[0].as<uint64_t>());
                    chatMessage.set_senderaccount(row[1].as<std::string>());
                    chatMessage.set_receiveraccount(row[2].as<std::string>());
                    chatMessage.set_content(row[3].as<std::string>());
                    chatMessage.set_timestamp(kaka::Date(row[4].as<std::string>()).toString());
                    chatMessage.set_sign("");
                    chatMessageList.emplace_back(std::move(chatMessage));
                }

            } catch (std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "查询聊天记录失败," << exception.what());
            }

            //3.发送响应
            for (auto message: chatMessageList) {
                message.set_sessionid(pullOfflineMessage.sessionid());
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, message);
                }
            }
        }

        void OfflineModule::handlePullGroupChatMessage(const kakaIM::Node::PullGroupChatMessage &pullGroupChatMessage,
                                                       const std::string connectionIdentifier) {
            //1.获取用户账号和消息ID
            const std::string sessionID = pullGroupChatMessage.sessionid();
            const uint64_t messageID = pullGroupChatMessage.messageid();
            const std::string groupID = pullGroupChatMessage.groupid();
            auto queryUserAccountWithSessionService = this->mQueryUserAccountWithSessionServicePtr.lock();
            if (!queryUserAccountWithSessionService) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                << "无法执行，由于缺少querUserAccountWithSessionService");
                return;
            }
            auto userAccount = queryUserAccountWithSessionService->queryUserAccountWithSession(sessionID);

            //2.判断此用户是否是否属于此群的成员


            //3.查询离线消息

            std::list<Node::GroupChatMessage> chatMessageList;
            const std::string QueryOfflineGroupChatMessageSQLStatement = "QueryOfflineGroupChatMessageSQL";
            const std::string queryOfflineGroupChatMessageSQL = "SELECT msg_sender,msg_content,msg_id FROM group_chat_timeline WHERE group_id = $1 AND msg_id > $2 ;";

            auto dbConnection = this->getDBConnection();

            if (!dbConnection) {
                //异常处理
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接失败");
                return;
            }
            try {

                pqxx::work dbWork(*dbConnection);

                auto invocation = dbWork.prepared(QueryOfflineGroupChatMessageSQLStatement);

                if (!invocation.exists()) {
                    dbConnection->prepare(QueryOfflineGroupChatMessageSQLStatement, queryOfflineGroupChatMessageSQL);
                }

                pqxx::result result = invocation(groupID)(messageID).exec();
                for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                    kakaIM::Node::GroupChatMessage chatMessage;
                    chatMessage.set_groupid(groupID);
                    chatMessage.set_sender(row[0].as<std::string>());
                    chatMessage.set_content(row[1].as<std::string>());
                    chatMessage.set_msgid(row[2].as<std::uint64_t>());
                    chatMessageList.emplace_back(std::move(chatMessage));
                }

            } catch (pqxx::sql_error exception) {
            }
            //3.发送响应
            for (auto message: chatMessageList) {
                message.set_sessionid(pullGroupChatMessage.sessionid());
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, message);
                }
            }
        }

        std::shared_ptr<pqxx::connection> OfflineModule::getDBConnection() {
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
                              typeid(this).name() << "" << __FUNCTION__ << "连接数据库异常，" << exception.what());
            }

            return this->m_dbConnection;
        }
    }
}
