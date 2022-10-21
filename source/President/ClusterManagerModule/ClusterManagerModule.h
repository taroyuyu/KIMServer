//
// Created by Kakawater on 2018/1/1.
//

#ifndef KAKAIMCLUSTER_CLUSTERMANAGERMODULE_H
#define KAKAIMCLUSTER_CLUSTERMANAGERMODULE_H

#include <President/KIMPresidentModule/KIMPresidentModule.h>
#include <map>
#include <unistd.h>
#include <Common/proto/MessageCluster.pb.h>
#include <Common/Net/MessageCenterModule/MessageFilter.h>
#include <President/ClusterManagerModule/ClusterEvent.h>
#include <Common/Net/MessageCenterModule/ConnectionDelegate.h>
#include <President/President.h>

namespace kakaIM {
    namespace president {
        class ClusterManagerModule
                : public KIMPresidentModule,
                  public net::MessageFilter,
                  public net::ConnectionDelegate,
                  public ServerManageService {
        public:
            ClusterManagerModule(const std::string invitation_code);

            ~ClusterManagerModule();

            virtual bool
            doFilter(const ::google::protobuf::Message &message, const std::string connectionIdentifier) override;

            virtual std::set<Node> getAllNodes() override;

            virtual Node getNode(std::string serverID) throw(NodeNotExitException) override;

            virtual Node getNodeWithConnectionIdentifier(std::string connectionIdentifier)throw(NodeNotExitException) override ;

            void addEventListener(ClusterEvent::ClusterEventType eventType, std::function<void(ClusterEvent)> callback);

            virtual void didCloseConnection(const std::string connectionIdentifier) override;
            virtual const std::unordered_set<std::string> & messageTypes() override;
        protected:
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task) override;
            std::unordered_set<std::string> mMessageTypeSet;
        private:
            const std::string invitation_code;


            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message> ,const std::string>> mTaskQueue;

            void handleRequestJoinClusterMessage(const RequestJoinClusterMessage &, const std::string connectionIdentifier);

            void handleHeartBeatMessage(const HeartBeatMessage &, const std::string connectionIdentifier);

            bool validateInvitationCode(std::string invitationCode);

            bool validateServerID(std::string serverID);

            void triggerEvent(ClusterEvent event);

            /**
             * key-value形式为serverID-connectionIdentifier
             */
            std::map<std::string, Node> nodeConnection;
            std::multimap<ClusterEvent::ClusterEventType, std::function<void(ClusterEvent)>> eventCallbacks;
        };
    }
}

#endif //KAKAIMCLUSTER_CLUSTERMANAGERMODULE_H

