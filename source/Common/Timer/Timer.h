//
// Created by 凌宇 on 2022/10/16.
//

#ifndef KAKAIMCLUSTER_TIMER_H
#define KAKAIMCLUSTER_TIMER_H

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
        void setTimeout(std::function<void()> task, int delay);
        void setInterval(std::function<void()> task, int interval);
        void stop();
    };
}


#endif //KAKAIMCLUSTER_TIMER_H
