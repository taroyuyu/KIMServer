//
// Created by Kakawater on 2018/3/16.
//

#include <string>
#include <list>
#ifndef KAKAIMCLUSTER_USERSTATEMANAGERSERVICE_H
#define KAKAIMCLUSTER_USERSTATEMANAGERSERVICE_H
namespace kakaIM {
    namespace president {
        class UserStateManagerService {
        public:
            /**
             * 查询指定用户在哪些服务器上进行登陆
             * @param userAccount
             * @return 服务器ID列表
             */
            virtual std::list<std::string> queryUserLoginServer(std::string userAccount) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_USERSTATEMANAGERSERVICE_H

