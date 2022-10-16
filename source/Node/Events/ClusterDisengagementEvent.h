//
// Created by Kakawater on 2018/3/22.
//

#ifndef KAKAIMCLUSTER_CLUSTERDISENGAGEMENTEVENT_H
#define KAKAIMCLUSTER_CLUSTERDISENGAGEMENTEVENT_H
#include "<Common/EventBus/Event.h>
namespace kakaIM {
    class ClusterDisengagementEvent : public Event {
    public:
        ClusterDisengagementEvent(){

        }
        virtual std::string getEventType() const override {
            return typeid(this).name();
        }
    };
}
#endif //KAKAIMCLUSTER_CLUSTERDISENGAGEMENTEVENT_H
