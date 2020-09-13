//
// Created by taroyuyu on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_ROSTERMODULE_H
#define KAKAIMCLUSTER_ROSTERMODULE_H

#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../Service/SessionQueryService.h"
#include "../Service/MessageSendService.h"
#include "../Service/UserRelationService.h"
#include "../../Common/KIMDBConfig.h"
#include "../Service/ConnectionOperationService.h"
namespace kakaIM {
    namespace node {
        class RosterModule
                : public common::KIMModule, public UserRelationService{
        public:
            RosterModule();

            ~RosterModule();

            virtual bool init() override;

            void setDBConfig(const common::KIMDBConfig &dbConfig);

            virtual bool checkFriendRelation(const std::string userA, const std::string userB) override;

            void addBuildingRelationshipRequestMessage(const kakaIM::Node::BuildingRelationshipRequestMessage &message,
                                                       const std::string connectionIdentifier);

            void addBuildingRelationshipAnswerMessage(const kakaIM::Node::BuildingRelationshipAnswerMessage &message,
                                                      const std::string connectionIdentifier);

            void
            addDestroyingRelationshipRequestMessage(const kakaIM::Node::DestroyingRelationshipRequestMessage &message,
                                                    const std::string connectionIdentifier);

            void addFriendListRequestMessage(const kakaIM::Node::FriendListRequestMessage &message,
                                             const std::string connectionIdentifier);

            void addFetchUserVCardMessage(const kakaIM::Node::FetchUserVCardMessage &message,
                                          const std::string connectionIdentifier);

            void addUpdateUserVCardMessage(const kakaIM::Node::UpdateUserVCardMessage &message,
                                           const std::string connectionIdentifier);

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void setQueryUserAccountWithSessionService(
                    std::weak_ptr<QueryUserAccountWithSession> queryUserAccountWithSessionServicePtr);

            void setMessageSendService(std::weak_ptr<MessageSendService> messageSendServicePtr);

        protected:
            virtual void execute() override;

        protected:
            int mEpollInstance;
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<QueryUserAccountWithSession> mQueryUserAccountWithSessionServicePtr;
            std::weak_ptr<MessageSendService> mMessageSendServicePtr;
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, std::string>> messageQueue;

            void
            handleBuildingRelationshipRequestMessage(const kakaIM::Node::BuildingRelationshipRequestMessage &message,
                                                     const std::string connectionIdentifier);

            void handleBuildingRelationshipAnswerMessage(const kakaIM::Node::BuildingRelationshipAnswerMessage &message,
                                                         const std::string connectionIdentifier);

            bool checkUserRelationApplication(const std::string sponsorAccount, const std::string targetAccount,
                                              const uint64_t applicantId);

            bool updateUserRelationApplicationState(const uint64_t applicantId,
                                                    kakaIM::Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer answer);

            void handleDestroyingRelationshipRequestMessage(
                    const kakaIM::Node::DestroyingRelationshipRequestMessage &message,
                    const std::string connectionIdentifier);

            void handleFriendListRequestMessage(const kakaIM::Node::FriendListRequestMessage &message,
                                                const std::string connectionIdentifier);

            void handleFetchUserVCardMessage(const kakaIM::Node::FetchUserVCardMessage &message,
                                             const std::string connectionIdentifier);

            void handleUpdateUserVCardMessage(const kakaIM::Node::UpdateUserVCardMessage &message,
                                              const std::string connectionIdentifier);

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

#endif //KAKAIMCLUSTER_ROSTERMODULE_H
