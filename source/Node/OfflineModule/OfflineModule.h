//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_OFFLINEMODULE_H
#define KAKAIMCLUSTER_OFFLINEMODULE_H

#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../Service/MessagePersistenceService.h"
#include "../Service/SessionQueryService.h"
#include "../Service/ConnectionOperationService.h"
#include "../../Common/KIMDBConfig.h"

namespace kakaIM {
    namespace node {
        class OfflineModule : public common::KIMModule, public MessagePersistenceService {
        public:
            OfflineModule();

            ~OfflineModule();

            virtual bool init() override;

            void setDBConfig(const common::KIMDBConfig &dbConfig);

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void setQueryUserAccountWithSessionService(
                    std::weak_ptr<QueryUserAccountWithSession> queryUserAccountWithSessionServicePtr);

            void addPullChatMessage(std::unique_ptr<kakaIM::Node::PullChatMessage> message, const std::string connectionIdentifier);

            void addPullGroupChatMessage(std::unique_ptr<kakaIM::Node::PullGroupChatMessage> message,
                                         const std::string connectionIdentifier);

            /**
             * 将消息持久化
             * @param message 待持久化的消息
             * @param messageID 消息ID
             */
            virtual void persistChatMessage(std::string userAccount, const kakaIM::Node::ChatMessage &message,
                                            const uint64_t messageID) override;

            virtual void persistGroupChatMessage(const kakaIM::Node::GroupChatMessage &groupChatMessage,
                                                 const uint64_t messageID) override;

        protected:
            virtual void execute() override;

        private:
            class PersistTask {
            public:
                virtual std::string getTaskName() const = 0;
            };

            class ChatMessagePersistTask : public PersistTask {
            public:
                ChatMessagePersistTask(std::string userAccount, const kakaIM::Node::ChatMessage &message,
                                       const uint64_t messageID) :
                        userAccount(userAccount), message(message), messageID(messageID) {

                }

                virtual std::string getTaskName() const override {
                    return typeid(*this).name();
                }

                std::string getUserAccount() const {
                    return this->userAccount;
                }

                const kakaIM::Node::ChatMessage &getMessage() const {
                    return this->message;
                }

                uint64_t getMessageID() const {
                    return this->messageID;
                }

            private:
                const std::string userAccount;
                const kakaIM::Node::ChatMessage message;
                const uint64_t messageID;
            };

            class GroupChatMessagePersistTask : public PersistTask {
            public:
                GroupChatMessagePersistTask(std::string groupId, const kakaIM::Node::GroupChatMessage &groupChatMessage,
                                            const uint64_t messageID) :
                        _groupId(groupId), _message(groupChatMessage), _messageID(messageID) {

                }

                virtual std::string getTaskName() const {
                    return typeid(*this).name();
                }

                std::string getGroupId() const {
                    return this->_groupId;
                }

                const kakaIM::Node::GroupChatMessage &getMessage() const {
                    return this->_message;
                }

                uint64_t getMessageID() const {
                    return this->_messageID;
                }

            private:
                const std::string _groupId;
                const kakaIM::Node::GroupChatMessage _message;
                const uint64_t _messageID;

            };

            int mEpollInstance;
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> messageQueue;

            void handleChatMessagePersist(std::string userAccount, const kakaIM::Node::ChatMessage &message,
                                          const uint64_t messageID);

            void handletGroupChatMessagePersis(const std::string groupId, const kakaIM::Node::GroupChatMessage &message,
                                               const uint64_t messageID);

            int persistTaskQueueEventfd;
            std::mutex persistTaskQueueMutex;
            std::queue<std::unique_ptr<PersistTask>> persistTaskQueue;

            void
            handlePullChatMessage(const kakaIM::Node::PullChatMessage &pullOfflineMessage,
                                  const std::string connectionIdentifier);

            void handlePullGroupChatMessage(const kakaIM::Node::PullGroupChatMessage &pullGroupChatMessage,
                                            const std::string connectionIdentifier);

            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<QueryUserAccountWithSession> mQueryUserAccountWithSessionServicePtr;
            /**
             * @description 数据库连接
             */
            common::KIMDBConfig dbConfig;
	    std::shared_ptr<pqxx::connection> m_dbConnection;

	    std::shared_ptr<pqxx::connection> getDBConnection();
	    log4cxx::LoggerPtr logger;
        };

    }
}

#endif //KAKAIMCLUSTER_OFFLINEMODULE_H
