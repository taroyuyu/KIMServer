//
// Created by Kakawater on 2018/3/14.
//

#ifndef KAKAIMCLUSTER_USERRELATIONSERVICE_H
#define KAKAIMCLUSTER_USERRELATIONSERVICE_H

#include <list>
namespace kakaIM {
    namespace node {
        class UserRelationService {
        public:
            /**
             * 验证userA和userB是否存在好友关系
             * @param userA
             * @param userB
             * @return 存在好友关系，则返回true,否则返回false
             */
            virtual bool checkFriendRelation(const std::string userA, const std::string userB) = 0;

            /**
             * 获取用户的好友列表
             * @param userAccount 
             * @return 
             */
	    virtual std::list<std::string> retriveUserFriendList(const std::string userAccount) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_USERRELATIONSERVICE_H

