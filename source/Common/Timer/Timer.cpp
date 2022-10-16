//
// Created by 凌宇 on 2022/10/16.
//

#include <Common/Timer/Timer.h>
#include <chrono>

namespace kakaIM {
    void Timer::setTimeout(std::function<void()> task, int delay) {
        std::lock_guard<std::mutex> lock(this->mutex);
        if (this->active){
            return;
        }

        if (this->workThread.joinable()){
            this->workThread.join();
        }

        this->active = true;
        std::thread thread([=]() {
            if (not this->active) {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            if (not this->active) {
                return;
            }
            task();
            this->active = false;
        });
        this->workThread = std::move(thread);
    }

    void Timer::setInterval(std::function<void()> task, int interval) {
        std::lock_guard<std::mutex> lock(this->mutex);
        if (this->active){
            return;
        }

        if (this->workThread.joinable()){
            this->workThread.join();
        }

        this->active = true;
        std::thread thread([=]() {
            while (this->active) {
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
                if (not this->active) {
                    break;
                }
                task();
            }
        });
        this->workThread = std::move(thread);
    }

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