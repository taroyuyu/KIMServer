//
// Created by Kakawater on 2018/1/26.
//

#ifndef KAKAIMCLUSTER_MESSAGEIDGENERATESERVICE_H
#define KAKAIMCLUSTER_MESSAGEIDGENERATESERVICE_H

#include <memory>
#include <Common/util/Date.h>

namespace kakaIM {
    namespace node {
        class MessageIDGenerateService {
        public:
            class Futrue {
            public:
                virtual uint64_t getMessageID() = 0;
            };

            /**
             * 在指定的用户范围内生成一个全局唯一的消息ID
             * @param userAccount 用户账号
             * @return
             */
            virtual std::shared_ptr<Futrue> generateMessageIDWithUserAccount(const std::string &userAccount) = 0;
        };
    }
}

#endif //KAKAIMCLUSTER_MESSAGEIDGENERATESERVICE_H

