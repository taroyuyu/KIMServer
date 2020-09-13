//
// Created by taroyuyu on 2018/1/8.
//

#ifndef KAKAIMCLUSTER_CLUSTEREVENT_H
#define KAKAIMCLUSTER_CLUSTEREVENT_H

#include <string>

namespace kakaIM {
    namespace president {
        class ClusterEvent {
        public:
            enum ClusterEventType{
                NewNodeJoinedCluster,//新的节点加入集群
                NodeRemovedCluster,//节点脱离集群
            };
            ClusterEvent(std::string targetServerID,ClusterEventType eventType):
                    eventType(eventType),targetServerID(targetServerID)
            {
            };
            ClusterEventType getEventType()const
            {
                return this->eventType;
            }
            std::string getTargetServerID()const
            {
                return this->targetServerID;
            }
        private:
            ClusterEventType eventType;
            std::string targetServerID;
        };
    }
}

#endif //KAKAIMCLUSTER_CLUSTEREVENT_H
