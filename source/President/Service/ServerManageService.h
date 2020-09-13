//
// Created by taroyuyu on 2018/2/25.
//

#ifndef KAKAIMCLUSTER_SERVERMANAGESERVICE_H
#define KAKAIMCLUSTER_SERVERMANAGESERVICE_H

#include <set>
#include "Node.h"

namespace kakaIM {
    namespace president {
        class ServerManageService {
        public:
            /**
             * 获取所有的节点(成功加入此集群的节点)
             * @return
             */
            virtual std::set<Node> getAllNodes() = 0;

            class NodeNotExitException : public std::exception {
            public:
                NodeNotExitException(std::string serverID) : serverID(serverID) {

                }

                std::string serverID;
            };

            /**
             * 根据serverID获取节点信息
             * @param serverID
             * @return
             */
            virtual Node getNode(std::string serverID) throw(NodeNotExitException) = 0;

            /**
                 * 根据connectionIdentifier获取节点信息
                 * @param connectionIdentifier
                 * @return
                 */
            virtual Node
            getNodeWithConnectionIdentifier(std::string connectionIdentifier)throw(NodeNotExitException) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_SERVERMANAGESERVICE_H

