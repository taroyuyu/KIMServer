//
// Created by Kakawater on 2018/3/22.
//

#ifndef KAKAIMCLUSTER_NODESECESSIONEVENT_H
#define KAKAIMCLUSTER_NODESECESSIONEVENT_H
#include "../../Common/EventBus/Event.h"
namespace kakaIM {
    class NodeSecessionEvent : public Event {
    public:
        NodeSecessionEvent(const std::string & serverID) : _serverID(serverID){

        };

        virtual std::string getEventType() const override {
            return typeid(this).name();
        }

        std::string getServerID()const{
            return this->_serverID;
        }
    private:
        std::string _serverID;
    };
}
#endif //KAKAIMCLUSTER_NODESECESSIONEVENT_H
