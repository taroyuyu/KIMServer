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
            virtual void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) override;
            virtual void setMessageSendService(std::weak_ptr<MessageSendService> messageSendServicePtr) override;
            virtaul void setClusterService(std::weak_ptr<ClusterService> service) override;
        protected:
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<MessageSendService> mMessageSendServicePtr;
            std::weak_ptr<ClusterService> mClusterServicePtr;
        };
    }
}


#endif //KAKAIMCLUSTER_KIMNODEMODULE_H
