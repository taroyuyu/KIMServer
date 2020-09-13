//
// Created by taroyuyu on 2018/1/11.
//

#ifndef KAKAIMCLUSTER_SERVERRELAYMODULE_H
#define KAKAIMCLUSTER_SERVERRELAYMODULE_H

#include <queue>
#include <mutex>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../../Common/proto/MessageCluster.pb.h"
#include "../ClusterManagerModule/ClusterEvent.h"
#include "../Service/ConnectionOperationService.h"
#include "../Service/UserStateManagerService.h"
#include "../Service/ServerManageService.h"
namespace kakaIM {
    namespace president {
        class ServerRelayModule : public kakaIM::common::KIMModule {
        public:
            ServerRelayModule();

            ~ServerRelayModule();

            virtual bool init() override;

            virtual void execute() override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void setUserStateManagerService(std::weak_ptr<UserStateManagerService> userStateManagerServicePtr);

            void setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr);

            void addServerMessage(const ServerMessage &message, const std::string connectionIdentifier);

            void addEvent(ClusterEvent event);

        private:
            void handleServerMessage(const ServerMessage &message, const std::string connectionIdentifier);

            void handleNewNodeJoinedClusterEvent(const ClusterEvent &event);

            void handleNodeRemovedClusterEvent(const ClusterEvent &event);

            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<UserStateManagerService> mUserStateManagerServicePtr;
            std::weak_ptr<ServerManageService> mServerManageServicePtr;

            int mEpollInstance;

            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<ServerMessage>, const std::string>> messageQueue;

            int eventQueuefd;
            std::mutex eventQueueMutex;
            std::queue<ClusterEvent> mEventQueue;

	    log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_SERVERRELAYMODULE_H

