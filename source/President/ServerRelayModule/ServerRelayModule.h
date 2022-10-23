//
// Created by Kakawater on 2018/1/11.
//

#ifndef KAKAIMCLUSTER_SERVERRELAYMODULE_H
#define KAKAIMCLUSTER_SERVERRELAYMODULE_H

#include <President/KIMPresidentModule/KIMPresidentModule.h>
#include <Common/proto/MessageCluster.pb.h>
#include <President/ClusterManagerModule/ClusterEvent.h>
#include <functional>

namespace kakaIM {
    namespace president {
        class ServerRelayModule : public KIMPresidentModule {
        public:
            ServerRelayModule();

            ~ServerRelayModule();

            void addEvent(ClusterEvent event);

            virtual const std::unordered_set<std::string> messageTypes() override;
        protected:
            virtual void execute() override;
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task)override;
            std::unordered_map<std::string,std::function<void(std::unique_ptr<::google::protobuf::Message>, const std::string)>> mMessageHandlerSet;
        private:
            ConcurrentLinkedQueue<ClusterEvent> mEventQueue;
            void dispatchClusterEvent(ClusterEvent & event);
            void handleServerMessage(const ServerMessage &message, const std::string connectionIdentifier);

            void handleNewNodeJoinedClusterEvent(const ClusterEvent &event);

            void handleNodeRemovedClusterEvent(const ClusterEvent &event);
        };
    }
}


#endif //KAKAIMCLUSTER_SERVERRELAYMODULE_H

