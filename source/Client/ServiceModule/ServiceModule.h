//
// Created by Kakawater on 2018/2/1.
//

#ifndef KAKAIMCLUSTER_SERVICEMODULE_H
#define KAKAIMCLUSTER_SERVICEMODULE_H

#include <google/protobuf/message.h>
#include <list>
#include <Client/SessionModule/SessionModule.h>
namespace kakaIM {
    namespace client {
        class SessionModule;
        class ServiceModule {
            friend class SessionModule;

        public:
            virtual ~ServiceModule(){

            }
        protected:
            virtual void setSessionModule(SessionModule & sessionModule) = 0;
            virtual std::list<std::string> messageTypes() = 0;
            virtual void handleMessage(const ::google::protobuf::Message & message,SessionModule & sessionModule) = 0;
        };
    }
}


#endif //KAKAIMCLUSTER_SERVICEMODULE_H
