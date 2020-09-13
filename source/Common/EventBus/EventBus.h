//
// Created by taroyuyu on 2018/1/24.
//

#ifndef KAKAIMCLUSTER_EVENTBUS_H
#define KAKAIMCLUSTER_EVENTBUS_H

#include <memory>
#include <list>
#include <map>
#include "EventListener.h"
namespace kakaIM {
    class EventBus {
    public:
        EventBus();
        ~EventBus();
        /**
         * 获取默认的EventBus
         * @return
         */
        static EventBus & getDefault();

        void registerEvent(std::string eventType,EventListener * eventListenrPtr);
        void unregister(EventListener * eventListenrPtr,std::string eventType);
        void unregister(EventListener * eventListenrPtr);
        void Post(std::shared_ptr<const Event> event);
    private:
        static EventBus * defaultEventBus;
        mutable std::mutex m_mutex;
        std::map<std::string,std::list<EventListener*>> m_listenerMap;
    };
}

#endif //KAKAIMCLUSTER_EVENTBUS_H
