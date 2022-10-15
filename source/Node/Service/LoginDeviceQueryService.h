//
// Created by Kakawater on 2018/1/26.
//

#ifndef KAKAIMCLUSTER_LOGINDEVICEQUERYSERVICE_H
#define KAKAIMCLUSTER_LOGINDEVICEQUERYSERVICE_H

#include <set>
#include <string>
#include "../../Common/proto/KakaIMMessage.pb.h"

namespace kakaIM {
    namespace node {
        class LoginDeviceQueryService {
        public:
            enum IDType {
                IDType_SessionID,
                IDType_ServerID
            };

            /**
             * 根据用户账号，查询登陆设备集合 集合的元素描述为Set(<type,sessionID/serverID>,onlineState)
             * @param userAccount 用户账号
             * @return 返回集合的迭代器<begin,end>
             */
            virtual std::pair<std::set<std::pair<std::pair<IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator, std::set<std::pair<std::pair<IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator>
            queryLoginDeviceSetWithUserAccount(const std::string &userAccount) const = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_LOGINDEVICEQUERYSERVICE_H
