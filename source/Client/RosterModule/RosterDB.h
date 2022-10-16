//
// Created by Kakawater on 2018/2/26.
//

#ifndef KAKAIMCLUSTER_ROSTERDB_H
#define KAKAIMCLUSTER_ROSTERDB_H

#include <Client/RosterModule/User.h>
namespace kakaIM {
    namespace client {
        class RosterDB{
        public:
            ~RosterDB(){

            }
            /**
             * 更新用户电子名片
             * @param userVCard
             */
            virtual void updateUserVcard(UserVCard & userVCard) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_ROSTERDB_H
