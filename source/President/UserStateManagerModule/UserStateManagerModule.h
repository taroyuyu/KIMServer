//
// Created by taroyuyu on 2018/1/1.
//

#ifndef KAKAIMCLUSTER_USERSTATEMANAGERMODULE_H
#define KAKAIMCLUSTER_USERSTATEMANAGERMODULE_H

#include <queue>
#include <zconf.h>
#include <memory>
#include <mutex>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../../Common/proto/MessageCluster.pb.h"
#include "../ClusterManagerModule/ClusterEvent.h"
#include "../Service/ServerManageService.h"
#include "../Service/ConnectionOperationService.h"
#include "../Service/UserStateManagerService.h"

namespace kakaIM {
    namespace president {
        class UserStateManagerModule : public kakaIM::common::KIMModule, public UserStateManagerService {
        public:
            UserStateManagerModule();

            ~UserStateManagerModule();

            virtual bool init() override;

            virtual void execute() override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void addUpdateUserOnlineStateMessage(const UpdateUserOnlineStateMessage &message,
                                                 const std::string connectionIdentifier);

            void
            addUserOnlineStateMessage(const UserOnlineStateMessage &message, const std::string connectionIdentifier);

            void addEvent(ClusterEvent event);

            void setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr);

            /**
             * 查询指定用户在哪些服务器上进行登陆
             * @param userAccount
             * @return 服务器ID列表
             */
            std::list<std::string> queryUserLoginServer(std::string userAccount);

        private:
            int mEpollInstance;

            std::mutex messageQueueMutex;
            int messageEventfd;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> messageQueue;

            void handleUpdateUserOnlineStateMessage(const UpdateUserOnlineStateMessage &,
                                                    const std::string connectionIdentifier);

            void handleUserOnlineStateMessage(const UserOnlineStateMessage &, const std::string connectionIdentifier);

            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;

            int clusterEventfd;
            std::mutex eventQueueMutex;
            std::queue<ClusterEvent> mEventQueue;

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

            std::weak_ptr<ServerManageService> mServerManageServicePtr;

            log4cxx::LoggerPtr logger;
        };
    }
}

#endif //KAKAIMCLUSTER_USERSTATEMANAGERMODULE_H

