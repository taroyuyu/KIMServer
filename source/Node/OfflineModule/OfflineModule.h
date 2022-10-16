//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_OFFLINEMODULE_H
#define KAKAIMCLUSTER_OFFLINEMODULE_H

#include <queue>
#include <Node/KIMNodeModule/KIMNodeModule.h>

namespace kakaIM {
    namespace node {
        class OfflineModule : public KIMNodeModule, public MessagePersistenceService {
        public:
            OfflineModule();

            ~OfflineModule();

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
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task)override;
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

            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> mTaskQueue;
            ConcurrentLinkedQueue<std::unique_ptr<PersistTask>> mPersistTaskQueue;

            void dispatchPersistTask(const PersistTask & task);

            void handleChatMessagePersist(std::string userAccount, const kakaIM::Node::ChatMessage &message,
                                          const uint64_t messageID);

            void handletGroupChatMessagePersis(const std::string groupId, const kakaIM::Node::GroupChatMessage &message,
                                               const uint64_t messageID);

            void
            handlePullChatMessage(const kakaIM::Node::PullChatMessage &pullOfflineMessage,
                                  const std::string connectionIdentifier);

            void handlePullGroupChatMessage(const kakaIM::Node::PullGroupChatMessage &pullGroupChatMessage,
                                            const std::string connectionIdentifier);
        };

    }
}

#endif //KAKAIMCLUSTER_OFFLINEMODULE_H
