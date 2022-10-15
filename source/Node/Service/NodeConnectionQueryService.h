//
// Created by Kakawater on 2018/3/16.
//

#ifndef KAKAIMCLUSTER_NODECONNECTIONQUERYSERVICE_H
#define KAKAIMCLUSTER_NODECONNECTIONQUERYSERVICE_H
namespace kakaIM {
    namespace node {
        class NodeConnectionQueryService{
        public:
            virtual uint64_t queryCurrentNodeConnectionCount() = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_NODECONNECTIONQUERYSERVICE_H

