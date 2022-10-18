//
// Created by 凌宇 on 2022/10/18.
//

#ifndef KAKAIMCLUSTER_KIMPRESIDENTMODULE_H
#define KAKAIMCLUSTER_KIMPRESIDENTMODULE_H

#include <Common/KIMModule.h>
#include <log4cxx/logger.h>
#include <President/Service/ConnectionOperationService.h>
#include <President/Service/ServerManageService.h>
#include <President/Service/UserStateManagerService.h>

namespace kakaIM {
    namespace president {
        class KIMPresidentModule : public common::KIMModule {
        public:
            KIMPresidentModule(std::string moduleName);

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);
            void setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr);
            void setUserStateManagerService(std::weak_ptr<UserStateManagerService> userStateManagerServicePtr);
        protected:
            virtual void execute() override;
            virtual void shouldStop() override;
            std::atomic_bool m_needStop;
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task) = 0;

            log4cxx::LoggerPtr logger;
            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;
            std::weak_ptr<ServerManageService> mServerManageServicePtr;
            std::weak_ptr<UserStateManagerService> mUserStateManagerServicePtr;
        };
    }
}


#endif //KAKAIMCLUSTER_KIMPRESIDENTMODULE_H
