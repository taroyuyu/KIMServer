//
// Created by Kakawater on 2018/1/25.
//

#ifndef KAKAIMCLUSTER_USERLOGOUTEVENT_H
#define KAKAIMCLUSTER_USERLOGOUTEVENT_H

#include <typeinfo>
#include <Common/EventBus/Event.h>

namespace kakaIM {
    class UserLogoutEvent : public Event {
    public:
        UserLogoutEvent(std::string sessionID, std::string userAccount) : mSessionID(sessionID),
                                                                          mUserAccount(userAccount) {

        };

        virtual std::string getEventType() const override {
            return typeid(this).name();
        }

        /**
         * 获取用户使用的sessionID
         * 此sessionID在SessionModule中已经过期
         * @return
         */
        std::string getSessionID() const {
            return this->mSessionID;
        }

        std::string getUserAccount() const {
            return this->mUserAccount;
        }
    private:
        std::string mSessionID;
        std::string mUserAccount;
    };
}

#endif //KAKAIMCLUSTER_USERLOGOUTEVENT_H
