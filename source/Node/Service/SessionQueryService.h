//
// Created by Kakawater on 2018/1/25.
//

#ifndef KAKAIMCLUSTER_SESSIONQUERYSERVICE_H
#define KAKAIMCLUSTER_SESSIONQUERYSERVICE_H

namespace kakaIM {
    namespace node {
        class QueryUserAccountWithSession {
        public:
            /**
             * 根据sessionID查询userAccount
             * @param sessionID
             * @return 成功则返回userAccount,失败则返回空字符串
             */
            virtual std::string queryUserAccountWithSession(const std::string &sessionID) = 0;
        };

        class QueryConnectionWithSession {
        public:
            /**
             * 根据sessionID查询connection
             * @param sessionID
             * @return 成功则返回sessionID对应的connectionIdentifier,失败则返回空字符串
             */
            virtual std::string queryConnectionWithSession(const std::string &sessionID) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_SESSIONQUERYSERVICE_H
