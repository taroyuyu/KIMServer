//
// Created by 凌宇 on 2022/10/18.
//

#include <President/KIMPresidentModule/KIMPresidentModule.h>

namespace kakaIM {
    namespace president {
        KIMPresidentModule::KIMPresidentModule(std::string moduleName):logger(log4cxx::Logger::getLogger(moduleName)){

        }

        void KIMPresidentModule::execute(){
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (not this->m_needStop) {
                if (auto task = this->mTaskQueue.try_pop()) {
                    this->dispatchMessage(*task);
                } else {
                    std::this_thread::yield();
                }
            }

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }
        void KIMPresidentModule::shouldStop(){
            this->m_needStop = true;
        }

        void KIMPresidentModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void KIMPresidentModule::setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr) {
            this->mServerManageServicePtr = serverManageServicePtr;
        }

        void KIMPresidentModule::setUserStateManagerService(
                std::weak_ptr<UserStateManagerService> userStateManagerServicePtr) {
            this->mUserStateManagerServicePtr = userStateManagerServicePtr;
        }
    }
}