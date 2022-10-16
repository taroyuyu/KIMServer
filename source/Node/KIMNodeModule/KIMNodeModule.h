//
// Created by 凌宇 on 2022/10/15.
//

#ifndef KAKAIMCLUSTER_KIMNODEMODULE_H
#define KAKAIMCLUSTER_KIMNODEMODULE_H

#include "../../Common/KIMModule.h"
#include "../Service/ConnectionOperationService.h"
#include <memory>
namespace kakaIM {
    namespace node {
        class KIMNodeModule : public common::KIMModule {
        public:
            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);
        protected:
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
        };
    }
}


#endif //KAKAIMCLUSTER_KIMNODEMODULE_H
