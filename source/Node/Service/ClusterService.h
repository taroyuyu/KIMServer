//
// Created by Kakawater on 2018/1/26.
//

#ifndef KAKAIMCLUSTER_CLUSTERSERVICE_H
#define KAKAIMCLUSTER_CLUSTERSERVICE_H

#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../../Common/proto/MessageCluster.pb.h"

namespace kakaIM {
    namespace node {
        class ClusterService {
        public:
	    /**
             * 获取服务器Id
             * @return 
             */
            virtual std::string getServerID()const  = 0;
            /**
             * 向集群更新用户在线状态
             * @param userOnlineStateMessage
             */
            virtual void updateUserOnlineState(const kakaIM::Node::OnlineStateMessage &userOnlineStateMessage) = 0;

            class UserOnlineStateListener {
            public:
                virtual void didReceivedUserOnlineStateFromCluster(
                        const kakaIM::president::UserOnlineStateMessage &userOnlineStateMessage) = 0;
            };

            virtual void addUserOnlineStateListener(UserOnlineStateListener &userOnlineStateListener) = 0;

            virtual void removeUserOnlineStateListener(const UserOnlineStateListener &userOnlineStateListener) = 0;

            virtual void sendServerMessage(const kakaIM::president::ServerMessage &serverMessage) = 0;

            class ServerMessageListener{
            public:
                virtual void didReceivedServerMessageFromCluster(const kakaIM::president::ServerMessage & serverMessage) = 0;
            };
            virtual void addServerMessageListener(ServerMessageListener * listener) = 0;
            virtual void removeServerMessageListener(ServerMessageListener * listener) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_CLUSTERSERVICE_H

