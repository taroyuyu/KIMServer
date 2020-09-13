//
// Created by taroyuyu on 2018/1/17.
//

#ifndef KAKAIMCLUSTER_CONNECTIONDELEGATE_H
#define KAKAIMCLUSTER_CONNECTIONDELEGATE_H

namespace kakaIM {
    namespace net {
        class ConnectionDelegate {
        public:
            virtual ~ConnectionDelegate(){

            }
            virtual void didCloseConnection(const std::string expireConnectionIdentifier) = 0;
        };
    }
}

#endif //KAKAIMCLUSTER_CONNECTIONDELEGATE_H
