//
// Created by Kakawater on 2018/3/14.
//

#ifndef KAKAIMCLUSTER_CONNECTIONOPERATIONSERVICE_H
#define KAKAIMCLUSTER_CONNECTIONOPERATIONSERVICE_H

#include <google/protobuf/message.h>

namespace kakaIM {
    namespace president {
        class ConnectionOperationService {
        public:
            /**
             * 在指定连接上发送消息
             * @param connectionIdentifier
             * @param message
             * @return
             */
            virtual bool sendMessageThroughConnection(const std::string connectionIdentifier,
                                                      const ::google::protobuf::Message &message) = 0;
	    /**
             * 查询连接的地址信息
             * @param connectionIdentifier
             * @return
             */
            virtual std::pair<bool,std::pair<std::string,uint16_t>> queryConnectionAddress(const std::string connectionIdentifier) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_CONNECTIONOPERATIONSERVICE_H

