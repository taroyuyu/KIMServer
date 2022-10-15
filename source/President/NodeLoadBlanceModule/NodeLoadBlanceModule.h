//
// Created by Kakawater on 2018/3/16.
//

#ifndef KAKAIMCLUSTER_NODELOADBLANCEMODULE_H
#define KAKAIMCLUSTER_NODELOADBLANCEMODULE_H

#include <memory>
#include <mutex>
#include <queue>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../../Common/proto/MessageCluster.pb.h"
#include "../../Common/proto/KakaIMClientPresident.pb.h"
#include "../Service/ConnectionOperationService.h"
#include "../Service/ServerManageService.h"
#include "../Service/UserStateManagerService.h"
#include "../ClusterManagerModule/ClusterEvent.h"
#include "../../Common/ConcurrentQueue/ConcurrentLinkedQueue.h"

namespace kakaIM {
    namespace president {
        class NodeLoadBlanceModule : public kakaIM::common::KIMModule {
        public:
            NodeLoadBlanceModule();

            ~NodeLoadBlanceModule();

            virtual bool init() override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr);

            void setUserStateManagerService(std::weak_ptr<UserStateManagerService> userStateManagerServicePtr);

            void addNodeLoadInfoMessage(std::unique_ptr<NodeLoadInfoMessage> message, const std::string connectionIdentifier);

            void addRequestNodeMessage(std::unique_ptr<RequestNodeMessage> message, const std::string connectionIdentifier);

            void addEvent(ClusterEvent event);

        protected:
            virtual void execute() override;
            virtual void shouldStop() override;
            std::atomic_bool m_needStop;
        private:
            int mEpollInstance;

            std::mutex messageQueueMutex;
            int messageEventfd;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> messageQueue;

            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> mTaskQueue;
            ConcurrentLinkedQueue<ClusterEvent> mEventQueue;

            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task);
            void dispatchClusterEvent(ClusterEvent & event);

            void handleRequestNodeMessage(const RequestNodeMessage &message, const std::string connectionIdentifier);

            void handleNodeLoadInfoMessage(const NodeLoadInfoMessage &message, const std::string connectionIdentifier);

            std::weak_ptr<ConnectionOperationService> mConnectionOperationServicePtr;
            std::weak_ptr<UserStateManagerService> mUserStateManagerServicePtr;

            int clusterEventfd;
            std::mutex eventQueueMutex;
//            std::queue<ClusterEvent> mEventQueue;

            void handleNewNodeJoinedClusterEvent(const ClusterEvent &event);

            void handleNodeRemovedClusterEvent(const ClusterEvent &event);

            std::weak_ptr<ServerManageService> mServerManageServicePtr;

            class NodeLoadInfo {
            public:
                NodeLoadInfo() : connectionCount(0), cpuUsage(0), memUsage(0) {

                }

                NodeLoadInfo(uint64_t connectionCount, float cpuUsage, float memUsage) : connectionCount(
                        connectionCount), cpuUsage(cpuUsage), memUsage(memUsage) {

                }

                uint64_t connectionCount;//连接数
                float cpuUsage;//cpu利用率
                float memUsage;//内存使用率
            };

            std::mutex nodeLoadInfoSetMutex;
            std::map<Node, NodeLoadInfo> nodeLoadInfoSet;

            /**
                 * 计算地理上两点之间的距离
                 * @param point_1 (经度，纬度)
                 * @param point_2 (经度，纬度)
                 * @return
                 */
            int calculateDistance(std::pair<float, float> point_1, std::pair<float, float> point_2);

            log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_NODELOADBLANCEMODULE_H

