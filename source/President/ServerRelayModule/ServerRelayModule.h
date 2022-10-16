//
// Created by Kakawater on 2018/1/11.
//

#ifndef KAKAIMCLUSTER_SERVERRELAYMODULE_H
#define KAKAIMCLUSTER_SERVERRELAYMODULE_H

#include <queue>
#include <mutex>
#include <log4cxx/logger.h>
#include <Common/KIMModule.h>
#include <Common/proto/MessageCluster.pb.h>
#include <President/ClusterManagerModule/ClusterEvent.h>
#include <President/Service/ConnectionOperationService.h>
#include <President/Service/UserStateManagerService.h>
#include <President/Service/ServerManageService.h>
#include <Common/ConcurrentQueue/ConcurrentLinkedQueue.h>

namespace kakaIM {
    namespace president {
        class ServerRelayModule : public kakaIM::common::KIMModule {
        public:
            ServerRelayModule();

            ~ServerRelayModule();

            virtual bool init() override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void setUserStateManagerService(std::weak_ptr<UserStateManagerService> userStateManagerServicePtr);

            void setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr);

            void addEvent(ClusterEvent event);

        protected:
            virtual void execute() override;
            virtual void shouldStop() override;
            std::atomic_bool m_needStop;
        private:
            ConcurrentLinkedQueue<std::pair<std::unique_ptr<ServerMessage>, const std::string>> mTaskQueue;
            ConcurrentLinkedQueue<ClusterEvent> mEventQueue;
            void dispatchMessage(std::pair<std::unique_ptr<ServerMessage>, const std::string> & task);
            void dispatchClusterEvent(ClusterEvent & event);
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
//            std::queue<ClusterEvent> mEventQueue;

	    log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_SERVERRELAYMODULE_H

