//
// Created by 凌宇 on 2022/10/16.
//

#ifndef KAKAIMCLUSTER_TIMER_H
#define KAKAIMCLUSTER_TIMER_H

#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>

namespace kakaIM {
    class Timer {
    private:
        std::mutex mutex;
        std::atomic<bool> active{false};
        std::thread workThread;
    public:
        ~Timer();

        template<class Rep, class Period>
        void setTimeout(std::function<void()> task, const std::chrono::duration<Rep, Period> delay){
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
                std::this_thread::sleep_for(delay);
                if (not this->active) {
                    return;
                }
                task();
                this->active = false;
            });
            this->workThread = std::move(thread);
        }

        template<class Rep, class Period>
        void setInterval(std::function<void()> task, const std::chrono::duration<Rep, Period> interval) {
            std::lock_guard<std::mutex> lock(this->mutex);
            if (this->active) {
                return;
            }

            if (this->workThread.joinable()) {
                this->workThread.join();
            }

            this->active = true;
            std::thread thread([=]() {
                while (this->active) {
                    std::this_thread::sleep_for(interval);
                    if (not this->active) {
                        break;
                    }
                    task();
                }
            });
            this->workThread = std::move(thread);
        }

        void stop();
    };
}


#endif //KAKAIMCLUSTER_TIMER_H
