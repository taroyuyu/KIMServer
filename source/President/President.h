//
// Created by Kakawater on 2018/1/1.
//

#ifndef KAKAIMCLUSTER_PRESIDENT_H
#define KAKAIMCLUSTER_PRESIDENT_H

#include <semaphore.h>
#include <memory>
#include "Service/ConnectionOperationService.h"
#include "../Common/Net/MessageCenterModule/MessageCenterModule.h"

namespace kakaIM {
    namespace president {
        class ClusterManagerModule;
        class ServerRelayModule;
        class UserStateManagerModule;
	class MessageIDGenerateModule;
	class NodeLoadBlanceModule;

	class MessageCenterModule : public net::MessageCenterModule,public ConnectionOperationService{
        public:
            MessageCenterModule(std::string listenIP, u_int16_t listenPort,std::shared_ptr<net::MessageCenterAdapter> adapter,
                                size_t timeout = 0):net::MessageCenterModule(listenIP,listenPort,adapter,timeout){

            }
            virtual bool sendMessageThroughConnection(const std::string connectionIdentifier,
                                                      const ::google::protobuf::Message &message)override {
                return this->sendMessage(connectionIdentifier,message);
            }

	    virtual std::pair<bool,std::pair<std::string,uint16_t>> queryConnectionAddress(const std::string connectionIdentifier)override {
                return this->net::MessageCenterModule::queryConnectionAddress(connectionIdentifier);
            }
        };

        class President  {
        public:
            President();
            ~President();
	    bool init(int argc,char * argv[]);
            int start();
            std::shared_ptr<net::MessageCenterModule> getMessageCenterModule()
            {
                return this->mMessageCenterModulePtr;
            }

            std::shared_ptr<ClusterManagerModule> getClusterManagerModule()
            {
                return this->mClusterManagerModulePtr;
            }

            std::shared_ptr<UserStateManagerModule> getUserStateManagerModule()
            {
                return this->mUserStateManagerModulePtr;
            }
	    std::shared_ptr<MessageIDGenerateModule> getMessageIDGenerateModule(){
                return this->mMessageIDGenerateModulePtr;
            }
        private:
            sem_t *m_task_semaphore;
            std::shared_ptr<MessageCenterModule> mMessageCenterModulePtr;
            std::shared_ptr<ClusterManagerModule> mClusterManagerModulePtr;
            std::shared_ptr<ServerRelayModule> mServerRelayModulePtr;
            std::shared_ptr<UserStateManagerModule> mUserStateManagerModulePtr;
	    std::shared_ptr<MessageIDGenerateModule> mMessageIDGenerateModulePtr;
	    std::shared_ptr<NodeLoadBlanceModule> mNodeLoadBlanceModulePtr;
        };
    }
}


#endif //KAKAIMCLUSTER_PRESIDENT_H

