//
// Created by Kakawater on 2018/1/24.
//

#ifndef KAKAIMCLUSTER_EVENT_H
#define KAKAIMCLUSTER_EVENT_H

#include <string>

namespace kakaIM {
    class Event {
    public:
        virtual std::string getEventType() const = 0;

        virtual ~Event();
    };
}

#endif //KAKAIMCLUSTER_EVENT_H
