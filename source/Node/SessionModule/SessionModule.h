//
// Created by Kakawater on 2018/1/17.
//

#ifndef KAKAIMCLUSTER_SESSIONMODULE_H
#define KAKAIMCLUSTER_SESSIONMODULE_H

#include "../KIMNodeModule/KIMNodeModule.h"
#include "../../Common/Net/MessageCenterModule/ConnectionDelegate.h"
#include "../../Common/Net/MessageCenterModule/MessageFilter.h"
#include "../../Common/util/Date.h"
#include <map>
#include <queue>
#include <list>
#include <set>

namespace kakaIM {
    namespace node {
        class SessionModule : public KIMNodeModule, public net::ConnectionDelegate, public net::MessageFilter {
        public:
            SessionModule();

            virtual bool
            doFilter(const ::google::protobuf::Message &message, const std::string connectionIdentifier) override;

            virtual void didCloseConnection(const std::string connectionIdentifier) override;

        protected:
            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, std::string> &task);
        private:
            /**
             * key-Value形式为 sessionID-connectionIdentifier
             */
            std::map<std::string, std::string> sessionMap;

            void handleSessionIDRequestMessage(const kakaIM::Node::RequestSessionIDMessage &message,
                                               const std::string connectionIdentifier);


            std::string generateSessionID(const std::string connectionIdentifier);

            void
            handleLogoutMessage(const kakaIM::Node::LogoutMessage &message, const std::string connectionIdentifier);
        };
    }
}


#endif //KAKAIMCLUSTER_SESSIONMODULE_H
