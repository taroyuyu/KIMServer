//
// Created by Kakawater on 2018/2/16.
//

#ifndef KAKAIMCLUSTER_ONLINESTATEMODULE_H
#define KAKAIMCLUSTER_ONLINESTATEMODULE_H

#include "../ServiceModule/ServiceModule.h"
#include "../RosterModule/User.h"
#include <map>
namespace kakaIM {
    namespace client {
        class OnlineStateModule : public ServiceModule {
        public:
            OnlineStateModule();
            ~OnlineStateModule();
	    User::UserOnlineState getUserOnlineState(std::string userAccount);
        protected:
	    virtual void setSessionModule(SessionModule & sessionModule) override;

            virtual std::list<std::string> messageTypes() override;

	    virtual void handleMessage(const ::google::protobuf::Message & message,SessionModule & sessionModule) override;
        private:
            const SessionModule * sessionModule;
            /**
             * userOnlineDB的key-Value形式为"<userAccount>-<UserOnlineState>"
             */
            std::map<std::string,User::UserOnlineState> userOnlineDB;
        };
    }
}


#endif //KAKAIMCLUSTER_ONLINESTATEMODULE_H

