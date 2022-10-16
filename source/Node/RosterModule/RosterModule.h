//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_ROSTERMODULE_H
#define KAKAIMCLUSTER_ROSTERMODULE_H

#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>
#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include "../KIMNodeModule/KIMNodeModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../../Common/KIMDBConfig.h"
#include "../../Common/ConcurrentQueue/ConcurrentLinkedQueue.h"

namespace kakaIM {
    namespace node {
        class RosterModule
                : public KIMNodeModule, public UserRelationService {
        public:
            RosterModule();

            ~RosterModule();

            virtual bool init() override;

            virtual void start() override;

            virtual bool checkFriendRelation(const std::string userA, const std::string userB) override;

            virtual std::list<std::string> retriveUserFriendList(const std::string userAccount) override;

        protected:
            virtual void execute() override;

            virtual void shouldStop() override;

            std::atomic_bool m_needStop;
        protected:
            int mEpollInstance;
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, std::string>> messageQueue;
            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, std::string>> mTaskQueue;

            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, std::string> &task);

            void
            handleBuildingRelationshipRequestMessage(const kakaIM::Node::BuildingRelationshipRequestMessage &message,
                                                     const std::string connectionIdentifier);

            void handleBuildingRelationshipAnswerMessage(const kakaIM::Node::BuildingRelationshipAnswerMessage &message,
                                                         const std::string connectionIdentifier);

            enum UpdateFriendListVersionResult {
                UpdateFriendListVersionResult_DBConnectionNotExit,//数据库连接不存在
                UpdateFriendListVersionResult_InteralError,//内部错误
                UpdateFriendListVersionResult_Success,//查询成功
            };

            UpdateFriendListVersionResult
            updateFriendListVersion(const std::string userAccount, uint64_t &currentVersion);


            enum FetchFriendListVersionResult {
                FetchFriendListVersionResult_DBConnectionNotExit,//数据库连接不存在
                FetchFriendListVersionResult_InteralError,//内部错误
                FetchFriendListVersionResult_Success,//查询成功
                FetchFriendListVersionResult_RecordNotExist,//记录不存在
            };

            FetchFriendListVersionResult
            fetchFriendListVersion(const std::string userAccount, uint64_t &friendListVersion);

            bool checkUserRelationApplication(const std::string sponsorAccount, const std::string targetAccount,
                                              const uint64_t applicantId);

            bool updateUserRelationApplicationState(const uint64_t applicantId,
                                                    kakaIM::Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer answer,
                                                    std::string &handleTime);

            void handleDestroyingRelationshipRequestMessage(
                    const kakaIM::Node::DestroyingRelationshipRequestMessage &message,
                    const std::string connectionIdentifier);

            void handleFriendListRequestMessage(const kakaIM::Node::FriendListRequestMessage &message,

                                                const std::string connectionIdentifier);

            enum FetchFriendListResult {
                FetchFriendListResult_DBConnectionNotExit,//数据库连接不存在
                FetchFriendListResult_InteralError,//内部错误
                FetchFriendListResult_Success,//查询成功
            };

            FetchFriendListResult fetchFriendList(const std::string userAccount, std::set<std::string> &friendList);

            void handleFetchUserVCardMessage(const kakaIM::Node::FetchUserVCardMessage &message,
                                             const std::string connectionIdentifier);

            void handleUpdateUserVCardMessage(const kakaIM::Node::UpdateUserVCardMessage &message,
                                              const std::string connectionIdentifier);

            std::thread mRosterRPCWorkThread;
            AmqpClient::Channel::ptr_t mAmqpChannel;

            void rosterRPCListenerWork();

            std::shared_ptr<pqxx::connection> m_dbConnection;

            std::shared_ptr<pqxx::connection> getDBConnection();

            log4cxx::LoggerPtr logger;
        };

    }
}

#endif //KAKAIMCLUSTER_ROSTERMODULE_H
