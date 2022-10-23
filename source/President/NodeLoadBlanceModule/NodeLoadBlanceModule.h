//
// Created by Kakawater on 2018/3/16.
//

#ifndef KAKAIMCLUSTER_NODELOADBLANCEMODULE_H
#define KAKAIMCLUSTER_NODELOADBLANCEMODULE_H

#include <President/KIMPresidentModule/KIMPresidentModule.h>
#include <Common/proto/MessageCluster.pb.h>
#include <Common/proto/KakaIMClientPresident.pb.h>
#include <President/ClusterManagerModule/ClusterEvent.h>
#include <functional>

namespace kakaIM {
    namespace president {
        class NodeLoadBlanceModule : public KIMPresidentModule {
        public:
            NodeLoadBlanceModule();

            ~NodeLoadBlanceModule();

            void addEvent(ClusterEvent event);

            virtual const std::unordered_set<std::string> messageTypes() override;
        protected:
            virtual void execute() override;
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task)override;
            std::unordered_map<std::string,std::function<void(std::unique_ptr<::google::protobuf::Message>, const std::string)>> mMessageHandlerSet;
        private:
            ConcurrentLinkedQueue<ClusterEvent> mEventQueue;

            void dispatchClusterEvent(ClusterEvent & event);

            void handleRequestNodeMessage(const RequestNodeMessage &message, const std::string connectionIdentifier);

            void handleNodeLoadInfoMessage(const NodeLoadInfoMessage &message, const std::string connectionIdentifier);

            void handleNewNodeJoinedClusterEvent(const ClusterEvent &event);

            void handleNodeRemovedClusterEvent(const ClusterEvent &event);

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
        };
    }
}


#endif //KAKAIMCLUSTER_NODELOADBLANCEMODULE_H

