//
// Created by Kakawater on 2022/10/15.
//

#ifndef KAKAIMCLUSTER_KIMNODEMODULE_H
#define KAKAIMCLUSTER_KIMNODEMODULE_H

#include "../../Common/KIMModule.h"
#include "../Service/ConnectionOperationService.h"
#include "../Service/MessageSendService.h"
#include "../Service/ClusterService.h"
#include <memory>

namespace kakaIM {
    namespace node {
        class KIMNodeModule : public common::KIMModule {
        public:
            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);
            void setMessageSendService(std::weak_ptr<MessageSendService> messageSendServicePtr);
        protected:
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<MessageSendService> mMessageSendServicePtr;
        };
    }
}


#endif //KAKAIMCLUSTER_KIMNODEMODULE_H
