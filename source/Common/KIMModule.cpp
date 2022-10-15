//
// Created by Kakawater on 2018/1/2.
//

#include <iostream>
#include "KIMModule.h"

namespace kakaIM {
    namespace common {
        void KIMModule::start()
        {
            if (false == this->m_isStarted){
                this->m_isStarted = true;
                this->m_workThread = std::move(std::thread([this](){
                    this->execute();
                    this->m_isStarted = false;
                }));
            }
        }

        void KIMModule::stop() {
            if (true == this->m_isStarted){
                this->m_isStarted = false;
            }
        }


    }
}
