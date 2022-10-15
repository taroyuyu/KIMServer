//
// Created by Kakawater on 2018/1/17.
//

#ifndef KAKAIMCLUSTER_SESSIONMODULE_H
#define KAKAIMCLUSTER_SESSIONMODULE_H

#include "../../Common/KIMModule.h"
#include "../../Common/Net/MessageCenterModule/ConnectionDelegate.h"
#include "../../Common/Net/MessageCenterModule/MessageFilter.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../../Common/util/Date.h"
#include "../../Common/ConcurrentQueue/ConcurrentLinkedQueue.h"
#include "../Service/ConnectionOperationService.h"
#include <map>
#include <string>
#include <queue>
#include <list>
#include <set>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>

namespace kakaIM {
    namespace node {
        class SessionModule : public common::KIMModule, public net::ConnectionDelegate, public net::MessageFilter {
        public:
            SessionModule();

            ~SessionModule();

            virtual bool init() override;

            virtual bool
            doFilter(const ::google::protobuf::Message &message, const std::string connectionIdentifier) override;

            virtual void didCloseConnection(const std::string connectionIdentifier) override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

        protected:
            virtual void execute() override;
            virtual void shouldStop() override;
            std::atomic_bool m_needStop;
        private:
            int epollInstance;
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, std::string>> messageQueue;
            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, std::string>> mTaskQueue;


            /**
             * key-Value形式为 sessionID-connectionIdentifier
             */
            std::map<std::string, std::string> sessionMap;

            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, std::string> & task);

            void handleSessionIDRequestMessage(const kakaIM::Node::RequestSessionIDMessage &message,
                                               const std::string connectionIdentifier);


            std::string generateSessionID(const std::string connectionIdentifier);

            void
            handleLogoutMessage(const kakaIM::Node::LogoutMessage &message, const std::string connectionIdentifier);

            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
	    log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_SESSIONMODULE_H
