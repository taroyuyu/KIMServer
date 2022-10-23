//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_ONLINESTATEMODULE_H
#define KAKAIMCLUSTER_ONLINESTATEMODULE_H

#include <Node/KIMNodeModule/KIMNodeModule.h>
#include <map>
#include <Common/EventBus/EventListener.h>
#include <Node/Events/UserLogoutEvent.h>
#include <Node/Events/NodeSecessionEvent.h>
#include <functional>

namespace kakaIM {
    namespace node {
        class OnlineStateModule
                : public KIMNodeModule,
                  public EventListener,
                  public LoginDeviceQueryService,
                  public ClusterService::UserOnlineStateListener {
        public:
            OnlineStateModule();

            /**
             * @description 获取用户状态
             * @param userAccount 用户账号
             * @return 用户状态
             */
            kakaIM::Node::OnlineStateMessage_OnlineState getUserOnlineState(std::string userAccount);

            virtual void onEvent(std::shared_ptr<const Event> event) override;

            virtual void setClusterService(std::weak_ptr<ClusterService> clusterServicePtr) override;

            virtual std::pair<std::set<std::pair<std::pair<LoginDeviceQueryService::IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator, std::set<std::pair<std::pair<LoginDeviceQueryService::IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator>
            queryLoginDeviceSetWithUserAccount(const std::string &userAccount) const override;

            virtual void didReceivedUserOnlineStateFromCluster(
                    const kakaIM::president::UserOnlineStateMessage &userOnlineStateMessage) override;

            virtual const std::unordered_set<std::string> messageTypes() override;
        protected:
            virtual void execute() override;

            void
            dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> &task) override;
            std::unordered_map<std::string,std::function<void(std::unique_ptr<::google::protobuf::Message>, const std::string)>> mMessageHandlerSet;
        private:
            ConcurrentLinkedQueue<std::shared_ptr<const Event>> mEventQueue;

            void dispatchEvent(const Event &event);


            void handleOnlineMessage(const kakaIM::Node::OnlineStateMessage &onlineStateMessage,
                                     const std::string connectionIdentifier);

            void handleOnlineMessage(const kakaIM::president::UserOnlineStateMessage &onlineStateMessage);

            void handlePullFriendOnlineStateMessage(
                    const kakaIM::Node::PullFriendOnlineStateMessage &pullFriendOnlineStateMessage,
                    const std::string connectionIdentifier);

            void updateUserOnlineStateInThisNode(const std::string userAccount);

            void handleUserLogoutEvent(const UserLogoutEvent &event);

            void handleNodeSecessionEvent(const NodeSecessionEvent &event);


            /**
             * mUserStateDB的Key-value形式为:userAccount-Set(<type,sessionID/serverID>,onlineState)
             */
            std::map<std::string, std::set<std::pair<std::pair<IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>> mUserStateDB;
            std::unique_ptr<UserLogoutEvent> userLogoutEventProto;
            std::unique_ptr<NodeSecessionEvent> nodeSecessionEventProto;
        };
    }
}

#endif //KAKAIMCLUSTER_ONLINESTATEMODULE_H
