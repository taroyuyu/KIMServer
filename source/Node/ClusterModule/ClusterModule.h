//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_CLUSTERMODULE_H
#define KAKAIMCLUSTER_CLUSTERMODULE_H

#include <semaphore.h>
#include <map>
#include <time.h>
#include <functional>
#include <set>
#include <Node/KIMNodeModule/KIMNodeModule.h>
#include <Common/proto/MessageCluster.pb.h>
#include <Common/Net/TCPSocket/TCPClientSocket.h>
#include <Common/Net/TCPSocketManager/TCPSocketManager.h>
#include <Common/Net/TCPSocketManager/TCPSocketManagerConsignor.h>
#include <Common/KakaIMMessageAdapter.h>

namespace kakaIM {
    namespace node {
        class ClusterModule
                : public KIMNodeModule,
                  public kakaIM::net::TCPSocketManagerConsignor,
                  public ClusterService,
                  public MessageIDGenerateService {
        public:
            /**
             *
             * @param presidentAddr president的IP地址
             * @param presidentPort president监听的端口号
             * @param serverID Node的serverID
             * @param invitationCode 邀请码
	         * @param lngLatPair 经纬度(经度，纬度)
             * @param serviceAddr 本实例提供服务所监听的IP地址
             * @param servicePort 本实例提供服务所监听的端口
             */
            ClusterModule(std::string presidentAddr, int presidentPort, std::string serverID,
                          std::string invitationCode, std::pair<float, float> lngLatPair,std::string serviceAddr,
                          uint16_t servicePort);

            ~ClusterModule();

            virtual void stop() override;

            virtual void execute() override;

            virtual void didConnectionClosed(net::TCPClientSocket clientSocket) override;

            virtual void didReceivedMessage(int socketfd, const ::google::protobuf::Message &) override;

            std::string getServerID() const;

            virtual void updateUserOnlineState(const kakaIM::Node::OnlineStateMessage &userOnlineStateMessage) override;

            virtual void addUserOnlineStateListener(UserOnlineStateListener &userOnlineStateListener) override;

            virtual void removeUserOnlineStateListener(const UserOnlineStateListener &userOnlineStateListener) override;

            virtual void sendServerMessage(const kakaIM::president::ServerMessage &serverMessage)override;

            virtual void addServerMessageListener(ServerMessageListener * listener) override;

            virtual void removeServerMessageListener(ServerMessageListener * listener) override ;

            class MessageIDFutrue : public MessageIDGenerateService::Futrue {
                friend class ClusterModule;

            public:
                virtual uint64_t getMessageID() override;

                ~MessageIDFutrue();

            private:
                MessageIDFutrue();

                void setMessageID(uint64_t messageID);

                uint64_t messageID;
                bool ready;
                pthread_mutex_t conditionMutex;
                pthread_cond_t readyCondition;
            };

            virtual std::shared_ptr<MessageIDGenerateService::Futrue>
            generateMessageIDWithUserAccount(const std::string &userAccount) override;


        private:
            enum class ClusterModuleState {
                Disconnected,
                ConnectingPresident,
                ConnectedPresident,
            } moduleState;
            enum class MessageSource {
                NodeInternal,//节点内部
                MessageSource_Cluster,//集群
            };
            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, MessageSource>> mMessageQueue;
            std::list<UserOnlineStateListener *> mUserOnlineStateListenerList;

            void dispatchClusterMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, MessageSource> & pair);

            void handleHeartBeatEvent();

            void handleResponseJoinClusterMessage(const president::ResponseJoinClusterMessage &message);

            void handleResponseHeartBeatMessage(const president::ResponseHeartBeatMessage &message);

            void handleUpdateUserOnlineStateMessageFromCluster(const president::UserOnlineStateMessage &message);

            void handleUpdateUserOnlineStateMessageFromCluster(const president::UpdateUserOnlineStateMessage &message);

            void handleUpdateUserOnlineStateMessageFromNode(const kakaIM::Node::OnlineStateMessage &message);

            void handleServerMessageFromCluster(const president::ServerMessage &serverMessage);

            void handleServerMessageFromNode(const president::ServerMessage &serverMessage);

            net::TCPClientSocket mPresidentSocket;
            net::TCPSocketManager mSocketManager;
            common::KakaIMMessageAdapter *mKakaIMMessageAdapter;
            std::string mPresidentAddr;
            int mPresidentPort;
            std::string mServerID;
            std::string mInvitationCode;
            std::pair<float, float> mlngLatPair;
            std::string serviceAddr;
            uint16_t servicePort;
            int mHeartBeatTimerfd;//心跳定时器
            std::map<std::string, std::function<void(
                    kakaIM::president::ResponseMessageIDMessage response)>> mMessageIDRequestCallback;

            std::mutex serverMessageListenerSetMutex;
            std::set<ServerMessageListener*> serverMessageListenerSet;
        };
    }
}

#endif //KAKAIMCLUSTER_CLUSTERMODULE_H
