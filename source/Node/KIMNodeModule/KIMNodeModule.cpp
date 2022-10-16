//
// Created by 凌宇 on 2022/10/15.
//

#include "KIMNodeModule.h"

namespace kakaIM {
    namespace node {
        void KIMNodeModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }
    }
}