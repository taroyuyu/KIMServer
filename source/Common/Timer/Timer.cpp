//
// Created by 凌宇 on 2022/10/16.
//

#include <Common/Timer/Timer.h>

namespace kakaIM {
    void Timer::stop() {
        std::lock_guard<std::mutex> lock(this->mutex);
        if (not this->active){
            return;
        }
        this->active = false;
        this->workThread.join();
    }

    Timer::~Timer() {
        this->stop();
    }
}