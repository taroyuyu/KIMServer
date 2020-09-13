//
// Created by taroyuyu on 2018/1/17.
//

#ifndef KAKAIMCLUSTER_MESSAGEFILTER_H
#define KAKAIMCLUSTER_MESSAGEFILTER_H

#include <google/protobuf/message.h>
#include <memory>

namespace kakaIM {
    namespace net {
        class Connection;

        class MessageFilter {
        public:
            virtual bool
            doFilter(const ::google::protobuf::Message &message, const std::string connectionIdentifier) = 0;
        };
    }
}

#endif //KAKAIMCLUSTER_MESSAGEFILTER_H
