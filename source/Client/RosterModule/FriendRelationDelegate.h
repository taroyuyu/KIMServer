//
// Created by Kakawater on 2018/2/27.
//

#ifndef KAKAIMCLUSTER_FRIENDRELATIONDELEGATE_H
#define KAKAIMCLUSTER_FRIENDRELATIONDELEGATE_H

#include <Client/RosterModule/FriendRequest.h>
#include "User.h"
namespace kakaIM {
    namespace client {
        class FriendRelationDelegate{
        public:
            virtual void didReceiveFriendRequest(const FriendRequest & request) = 0;
            virtual void didReceiveFriendRequestAnswer(const FriendRequestAnswer & answer) = 0;
            virtual void didFriendShipRemoveByUser(const User & user) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_FRIENDRELATIONDELEGATE_H

