//
// Created by taroyuyu on 2018/1/1.
//

#ifndef KAKAIMCLUSTER_CLUSTERMANAGERMODULE_H
#define KAKAIMCLUSTER_CLUSTERMANAGERMODULE_H


#include <stdint.h>
#include <string>
#include <map>
#include <queue>
#include <unistd.h>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../../Common/proto/MessageCluster.pb.h"
#include "../../Common/Net/MessageCenterModule/MessageFilter.h"
#include "ClusterEvent.h"
#include "../../Common/Net/MessageCenterModule/ConnectionDelegate.h"
#include "../President.h"
#include "../Service/ServerManageService.h"
#include "../Service/ConnectionOperationService.h"
namespace kakaIM {
    namespace president {
        class ClusterManagerModule
                : public kakaIM::common::KIMModule,
                  public net::MessageFilter,
                  public net::ConnectionDelegate,
                  public ServerManageService {
        public:
            ClusterManagerModule();

            ~ClusterManagerModule();

            virtual bool init() override;

            virtual void execute() override;

            virtual bool
            doFilter(const ::google::protobuf::Message &message, const std::string connectionIdentifier) override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            virtual std::set<Node> getAllNodes() override;

            virtual Node getNode(std::string serverID) throw(NodeNotExitException) override;

            virtual Node getNodeWithConnectionIdentifier(std::string connectionIdentifier)throw(NodeNotExitException) override ;

            void addEventListener(ClusterEvent::ClusterEventType eventType, std::function<void(ClusterEvent)> callback);

            virtual void didCloseConnection(const std::string connectionIdentifier) override;

            void addRequestJoinClusterMessage(const RequestJoinClusterMessage &message, const std::string connectionIdentifier);

            void addHeartBeatMessage(const HeartBeatMessage &message, const std::string connectionIdentifier);
        private:
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;

            int messageEventfd;

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
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message> ,const std::string>> messageQueue;

	    log4cxx::LoggerPtr logger;
        };
    }
}

#endif //KAKAIMCLUSTER_CLUSTERMANAGERMODULE_H

