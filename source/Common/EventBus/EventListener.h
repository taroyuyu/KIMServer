//
// Created by Kakawater on 2018/1/24.
//

#ifndef KAKAIMCLUSTER_EVENTLISTENER_H
#define KAKAIMCLUSTER_EVENTLISTENER_H

#include "Event.h"
#include <memory>
namespace kakaIM {
    class EventListener {
    public:
        virtual ~EventListener(){

        }
        virtual void onEvent(std::shared_ptr<const Event> event) = 0;
    };
}
#endif //KAKAIMCLUSTER_EVENTLISTENER_H
