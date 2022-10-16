//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_ROSTERMODULE_H
#define KAKAIMCLUSTER_ROSTERMODULE_H

#include <Node/KIMNodeModule/KIMNodeModule.h>

namespace kakaIM {
    namespace node {
        class RosterModule
                : public KIMNodeModule, public UserRelationService {
        public:
            RosterModule();

            virtual bool checkFriendRelation(const std::string userA, const std::string userB) override;

            virtual std::list<std::string> retriveUserFriendList(const std::string userAccount) override;
        protected:
            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, std::string> &task);
        private:
            void
            handleBuildingRelationshipRequestMessage(const kakaIM::Node::BuildingRelationshipRequestMessage &message,
                                                     const std::string connectionIdentifier);

            void handleBuildingRelationshipAnswerMessage(const kakaIM::Node::BuildingRelationshipAnswerMessage &message,
                                                         const std::string connectionIdentifier);

            enum class UpdateFriendListVersionResult {
                DBConnectionNotExit,//数据库连接不存在
                InteralError,//内部错误
                Success,//查询成功
            };

            UpdateFriendListVersionResult
            updateFriendListVersion(const std::string userAccount, uint64_t &currentVersion);

            enum class FetchFriendListVersionResult {
                DBConnectionNotExit,//数据库连接不存在
                InteralError,//内部错误
                Success,//查询成功
                RecordNotExist,//记录不存在
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

            enum class FetchFriendListResult {
                DBConnectionNotExit,//数据库连接不存在
                InteralError,//内部错误
                Success,//查询成功
            };

            FetchFriendListResult fetchFriendList(const std::string userAccount, std::set<std::string> &friendList);

            void handleFetchUserVCardMessage(const kakaIM::Node::FetchUserVCardMessage &message,
                                             const std::string connectionIdentifier);

            void handleUpdateUserVCardMessage(const kakaIM::Node::UpdateUserVCardMessage &message,
                                              const std::string connectionIdentifier);
        };

    }
}

#endif //KAKAIMCLUSTER_ROSTERMODULE_H
