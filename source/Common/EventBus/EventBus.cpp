//
// Created by Kakawater on 2018/1/24.
//

#include <mutex>
#include "EventBus.h"
#include <iostream>
namespace kakaIM{
    EventBus * EventBus::defaultEventBus;

    EventBus & EventBus::getDefault(){
        static std::mutex mutex;

        if (nullptr == defaultEventBus) {
            std::lock_guard<std::mutex> lock(mutex);
            
            if (nullptr == defaultEventBus) {
                defaultEventBus = new EventBus();
            }
        }
        return *defaultEventBus;
    }

    EventBus::EventBus(){

    }
    EventBus::~EventBus(){

    }

    void EventBus::registerEvent(std::string eventType,EventListener* eventListenrPtr) {
        this->m_listenerMap[eventType].emplace_back(eventListenrPtr);
    }

    void EventBus::unregister(EventListener* eventListenrPtr,std::string eventType){
        std::lock_guard<std::mutex> lock(this->m_mutex);

        auto eventListenerIt =  this->m_listenerMap.find(eventType);

        if (eventListenerIt != this->m_listenerMap.end()){//存在该通知

            //移除监听者
            std::list<EventListener*> & eventListenerList = eventListenerIt->second;

            for (auto eventListenerPtrIt = eventListenerList.begin(); eventListenerPtrIt != eventListenerList.end();) {

                    if (*eventListenerPtrIt == eventListenrPtr) {
                        //移除
                        eventListenerPtrIt = eventListenerList.erase(eventListenerPtrIt);
                    }else{
                        ++eventListenerPtrIt;
                    }
            }

        }
    }
    void EventBus::unregister(EventListener* eventListenrPtr){
        for (auto it = this->m_listenerMap.begin();it != this->m_listenerMap.end(); ++it) {
            this->unregister(eventListenrPtr,it->first);
        }
    }

    void EventBus::Post(std::shared_ptr<const Event>  event){

        std::lock_guard<std::mutex>  lock(this->m_mutex);

        auto eventListenerIt =  this->m_listenerMap.find(event->getEventType());

        if (eventListenerIt != this->m_listenerMap.end()){
            std::list<EventListener*> & eventListenerList = eventListenerIt->second;
            for (auto eventListenItem = eventListenerList.begin(); eventListenItem != eventListenerList.end(); ++eventListenItem) {
                (*eventListenItem)->onEvent(event);
            }
        }

    }
}
