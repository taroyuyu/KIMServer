//
// Created by Kakawater on 2018/1/7.
//

#include <zconf.h>
#include "OfflineModule.h"
#include "../../Common/util/Date.h"
#include "../Log/log.h"
#include "../../Common/proto/KakaIMMessage.pb.h"

namespace kakaIM {
    namespace node {
        OfflineModule::OfflineModule() : KIMNodeModule(OfflineModuleLogger){
        }

        OfflineModule::~OfflineModule() {
            if (this->m_dbConnection && this->m_dbConnection->is_open()) {
                this->m_dbConnection->disconnect();
            }
        }

        void OfflineModule::execute() {
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (not this->m_needStop) {
                bool needSleep = true;
                if (auto task = this->mTaskQueue.try_pop()) {
                    this->dispatchMessage(*task);
                    needSleep = false;
                }

                if (auto task = this->mPersistTaskQueue.try_pop()){
                    this->dispatchPersistTask(**task);
                    needSleep = false;
                }

                if (needSleep){
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

        void OfflineModule::dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task){
            auto messageType = task.first->GetTypeName();
            if (messageType ==
                kakaIM::Node::PullChatMessage::default_instance().GetTypeName()) {
                this->handlePullChatMessage(
                        *(kakaIM::Node::PullChatMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::PullGroupChatMessage::default_instance().GetTypeName()) {
                this->handlePullGroupChatMessage(
                        *(kakaIM::Node::PullGroupChatMessage *) task.first.get(),
                        task.second);
            }
        }

        void OfflineModule::dispatchPersistTask(const PersistTask & task){
            if (0 == strcmp(typeid(task).name(), (typeid(ChatMessagePersistTask).name()))) {
                auto chatMessagePersistTask = static_cast<ChatMessagePersistTask &>(task);
                this->handleChatMessagePersist(chatMessagePersistTask->getUserAccount(),
                                               chatMessagePersistTask->getMessage(),
                                               chatMessagePersistTask->getMessageID());
            } else if (0 == strcmp(typeid(task).name(),
                                   typeid(GroupChatMessagePersistTask).name())) {
                auto groupChatMessagePersistTask = static_cast<GroupChatMessagePersistTask &>(task);
                this->handletGroupChatMessagePersis(groupChatMessagePersistTask->getGroupId(),
                                                    groupChatMessagePersistTask->getMessage(),
                                                    groupChatMessagePersistTask->getMessageID());
            }
        }

        void OfflineModule::persistChatMessage(std::string userAccount, const kakaIM::Node::ChatMessage &message,
                                               const uint64_t messageID) {
            std::unique_ptr<ChatMessagePersistTask> task(new ChatMessagePersistTask(userAccount, message, messageID));
            this->mPersistTaskQueue.push(std::move(task));
        }

        void OfflineModule::persistGroupChatMessage(const kakaIM::Node::GroupChatMessage &groupChatMessage,
                                                    const uint64_t messageID) {
            static size_t messageCount = 0;
            LOG4CXX_TRACE(this->logger, __FUNCTION__ << " messageCount=" << ++messageCount);
            std::unique_ptr<GroupChatMessagePersistTask> task(
                    new GroupChatMessagePersistTask(groupChatMessage.groupid(), groupChatMessage, messageID));
            this->mPersistTaskQueue.push(std::move(task));
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
    }
}
