//
// Created by taroyuyu on 2018/3/22.
//

#ifndef KAKAIMCLUSTER_NODESHUTDOWNEVENT_H
#define KAKAIMCLUSTER_NODESHUTDOWNEVENT_H
#include "../../Common/EventBus/Event.h"
namespace kakaIM {
    class NodeShutdownEvent : public Event {
    public:
        NodeShutdownEvent(){

        };

        virtual std::string getEventType() const override {
            return typeid(this).name();
        }
    };
}
#endif //KAKAIMCLUSTER_NODESHUTDOWNEVENT_H
