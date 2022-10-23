//
// Created by Kakawater on 2018/1/1.
//

#ifndef KAKAIMCLUSTER_USERSTATEMANAGERMODULE_H
#define KAKAIMCLUSTER_USERSTATEMANAGERMODULE_H

#include <President/KIMPresidentModule/KIMPresidentModule.h>
#include <zconf.h>
#include <Common/proto/MessageCluster.pb.h>
#include <President/ClusterManagerModule/ClusterEvent.h>
#include <functional>

namespace kakaIM {
    namespace president {
        class UserStateManagerModule : public KIMPresidentModule, public UserStateManagerService {
        public:
            UserStateManagerModule();

            ~UserStateManagerModule();

            void addEvent(ClusterEvent event);

            /**
             * 查询指定用户在哪些服务器上进行登陆
             * @param userAccount
             * @return 服务器ID列表
             */
            std::list<std::string> queryUserLoginServer(std::string userAccount);
            virtual const std::unordered_set<std::string> messageTypes() override;
        protected:
            virtual void execute() override;
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task)override;
            std::unordered_map<std::string,std::function<void(std::unique_ptr<::google::protobuf::Message>, const std::string)>> mMessageHandlerSet;
        private:
            ConcurrentLinkedQueue<ClusterEvent> mEventQueue;
            void dispatchClusterEvent(ClusterEvent & event);

            void handleUpdateUserOnlineStateMessage(const UpdateUserOnlineStateMessage &,
                                                    const std::string connectionIdentifier);

            void handleUserOnlineStateMessage(const UserOnlineStateMessage &, const std::string connectionIdentifier);

            void handleNewNodeJoinedClusterEvent(const ClusterEvent &event);

            void handleNodeRemovedClusterEvent(const ClusterEvent &event);

            /**
             * backedDB的Key-Value形式为:userAccount-set{(serverID,OnlineState)}
             */
            std::map<std::string, std::set<std::pair<std::string, UserOnlineStateMessage_OnlineState>>> mUserOnlineStateDB;

            /**
             * 更新用户在线状态
             * @param userAccount 用户在线状态
             * @param serverID 服务器ID
             * @param onlineState 在线状态
             */
            void updateUserOnlineState(std::string userAccount, std::string serverID,
                                       UserOnlineStateMessage_OnlineState onlineState);

            /**
             * 移除节点
             * @param serverID
             */
            void removeServer(const std::string serverID);
        };
    }
}

#endif //KAKAIMCLUSTER_USERSTATEMANAGERMODULE_H

