//
// Created by Kakawater on 2018/1/24.
//

#ifndef KAKAIMCLUSTER_GROUPCHATMODULE_H
#define KAKAIMCLUSTER_GROUPCHATMODULE_H

#include <pqxx/pqxx>
#include <queue>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>
#include "../KIMNodeModule/KIMNodeModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../../Common/KIMDBConfig.h"
#include "../../Common/ConcurrentQueue/ConcurrentLinkedQueue.h"

namespace kakaIM {
    namespace node {
        class GroupChatModule : public KIMNodeModule{
        public:
            GroupChatModule();

            ~GroupChatModule();

            virtual bool init() override;

            void setMessagePersistenceService(std::weak_ptr<MessagePersistenceService> service);

        protected:
            virtual void execute() override;
            virtual void shouldStop() override;
            std::atomic_bool m_needStop;
        private:
            int mEpollInstance;
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> messageQueue;

            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> mTaskQueue;

            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task);

            void handleChatGroupCreateRequestMessage(const kakaIM::Node::ChatGroupCreateRequest &message,
                                                     const std::string connectionIdentifier);

            void handleChatGroupDisbandRequestMessage(const kakaIM::Node::ChatGroupDisbandRequest &message,
                                                      const std::string connectionIdentifier);

            void handleChatGroupJoinRequestMessage(const kakaIM::Node::ChatGroupJoinRequest &message,
                                                   const std::string connectionIdentifier);

            std::pair<bool, uint64_t>
            persistChatGroupJoinApplication(const std::string applicant, const std::string group_id,
                                            const std::string introduction,std::string & submissionTime);

            /**
                 * 查询聊天群的管理员列表
                 * @param groupId
                 * @return
                 */
            std::list<std::string> queryChatGrouoManagerList(const std::string groupId);

            /**
                 * 检验用户对于特定聊天群是否具备管理权限
                 * @param groupID
                 * @param userAccount
                 * @return
                 */
            bool validateChatGroupManager(const std::string groupID, const std::string userAccount);

            /**
                 * 将特定用户加入聊天群
                 * @param userAccount
                 * @param groupID
                 * @return
                 */
            bool addUserToCharGroup(const std::string userAccount, const std::string groupID);

            void handleChatGroupJoinResponseMessage(const kakaIM::Node::ChatGroupJoinResponse &message,
                                                    const std::string connectionIdentifier);

            enum GroupJoinApplicationState {
                GroupJoinApplicationState_Pending,//待处理
                GroupJoinApplicationState_Allow,//通过
                GroupJoinApplicationState_Reject,//拒绝
            };
            struct GroupJoinApplication {
                uint64_t applicant_id;
                std::string applicant;
                uint64_t group_id;
                GroupJoinApplicationState state;
            };

            std::pair<bool, struct GroupJoinApplication> fetchGroupJoinApplicationInfo(const uint64_t applicantId);

            bool updateGroupJoinApplicationState(const uint64_t applicantId, enum GroupJoinApplicationState);

            void handleChatGroupQuitRequestMessage(const kakaIM::Node::ChatGroupQuitRequest &message,
                                                   const std::string connectionIdentifier);

            void handleUpdateChatGroupInfoRequestMessage(const kakaIM::Node::UpdateChatGroupInfoRequest &message,
                                                         const std::string connectionIdentifier);

            void handleFetchChatGroupInfoRequestMessage(const kakaIM::Node::FetchChatGroupInfoRequest &message,
                                                        const std::string connectionIdentifier);

            void
            handleFetchChatGroupMemberListRequestMessage(const kakaIM::Node::FetchChatGroupMemberListRequest &message,
                                                         const std::string connectionIdentifier);

            void handleFetchChatGroupListRequestMessage(const kakaIM::Node::FetchChatGroupListRequest &message,
                                                        const std::string connectionIdentifier);

            void
            handleGroupChatMessage(const kakaIM::Node::GroupChatMessage &message,
                                   const std::string connectionIdentifier);
            std::weak_ptr<MessagePersistenceService> mMessagePersistenceServicePtr;
            std::shared_ptr<pqxx::connection> m_dbConnection;

            std::shared_ptr<pqxx::connection> getDBConnection();

	    log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_GROUPCHATMODULE_H

