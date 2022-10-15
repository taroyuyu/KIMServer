//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_ONLINESTATEMODULE_H
#define KAKAIMCLUSTER_ONLINESTATEMODULE_H

#include "../../Common/KIMModule.h"
#include <google/protobuf/message.h>
#include <queue>
#include <map>
#include <memory>
#include <mutex>
#include <log4cxx/logger.h>
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../../Common/EventBus/EventListener.h"
#include "../Events/UserLogoutEvent.h"
#include "../Events/NodeSecessionEvent.h"
#include "../Service/SessionQueryService.h"
#include "../Service/LoginDeviceQueryService.h"
#include "../Service/UserRelationService.h"
#include "../Service/ClusterService.h"
#include "../Service/ConnectionOperationService.h"

namespace kakaIM {
    namespace node {
        class OnlineStateModule
                : public common::KIMModule,
                  public EventListener,
                  public LoginDeviceQueryService,
                  public ClusterService::UserOnlineStateListener {
        public:
            OnlineStateModule();

            ~OnlineStateModule();

            virtual bool init() override;

            /**
             * @description 添加在线状态消息
             * @param message
             */
            void addOnlineStateMessage(std::unique_ptr<kakaIM::Node::OnlineStateMessage> message,
                                       const std::string connectionIdentifier);
            /**
              * @description 添加拉取好友在线状态消息
              * @param message
              */
            void addPullFriendOnlineStateMessage(std::unique_ptr<kakaIM::Node::PullFriendOnlineStateMessage> message,const std::string connectionIdentifier);

            /**
             * @description 获取用户状态
             * @param userAccount 用户账号
             * @return 用户状态
             */
            kakaIM::Node::OnlineStateMessage_OnlineState getUserOnlineState(std::string userAccount);

            virtual void onEvent(std::shared_ptr<const Event> event) override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void setUserRelationService(std::weak_ptr<UserRelationService> userRelationServicePtr);

            void setClusterService(std::weak_ptr<ClusterService> clusterServicePtr);

            void setQueryUserAccountWithSessionService(
                    std::weak_ptr<QueryUserAccountWithSession> queryUserAccountWithSessionServicePtr);

            virtual std::pair<std::set<std::pair<std::pair<LoginDeviceQueryService::IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator, std::set<std::pair<std::pair<LoginDeviceQueryService::IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator>
            queryLoginDeviceSetWithUserAccount(const std::string &userAccount) const override;

            virtual void didReceivedUserOnlineStateFromCluster(
                    const kakaIM::president::UserOnlineStateMessage &userOnlineStateMessage) override;

        protected:
            virtual void execute() override;

        private:
            int mEpollInstance;
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<UserRelationService> userRelationServicePtr;
            std::weak_ptr<ClusterService> mClusterServicePtr;
            std::weak_ptr<QueryUserAccountWithSession> mQueryUserAccountWithSessionServicePtr;
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string >> messageQueue;
            int eventQueuefd;
            std::mutex eventQueueMutex;
            std::queue<std::shared_ptr<const Event>> mEventQueue;

            void handleOnlineMessage(const kakaIM::Node::OnlineStateMessage &onlineStateMessage,
                                     const std::string connectionIdentifier);

            void handleOnlineMessage(const kakaIM::president::UserOnlineStateMessage &onlineStateMessage);

            void handlePullFriendOnlineStateMessage(const kakaIM::Node::PullFriendOnlineStateMessage & pullFriendOnlineStateMessage,const std::string connectionIdentifier);

            void updateUserOnlineStateInThisNode(const std::string userAccount);

            void handleUserLogoutEvent(const UserLogoutEvent &event);

            void handleNodeSecessionEvent(const NodeSecessionEvent & event);


            /**
             * mUserStateDB的Key-value形式为:userAccount-Set(<type,sessionID/serverID>,onlineState)
             */
            std::map<std::string, std::set<std::pair<std::pair<IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>> mUserStateDB;
            std::unique_ptr<UserLogoutEvent> userLogoutEventProto;
            std::unique_ptr<NodeSecessionEvent> nodeSecessionEventProto;
            log4cxx::LoggerPtr logger;
        };
    }
}

#endif //KAKAIMCLUSTER_ONLINESTATEMODULE_H
