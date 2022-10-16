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
        std::atomic<bool> active{true};
        std::thread workThread;
    public:
        ~Timer();
        template <class Rep, class Period>
        void setTimeout(std::function<void()> task, const std::chrono::duration<Rep, Period> delay);
        template <class Rep, class Period>
        void setInterval(std::function<void()> task, const std::chrono::duration<Rep, Period> interval);
        void stop();
    };
}


#endif //KAKAIMCLUSTER_TIMER_H
