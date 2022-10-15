//
// Created by Kakawater on 2018/1/1.
//

#ifndef KAKAIMCLUSTER_KIMMODULE_H
#define KAKAIMCLUSTER_KIMMODULE_H

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace kakaIM {
    namespace common {
        class KIMModule {
        public:
            KIMModule() : m_status(Status::Stopped) {
            }

            virtual bool init() = 0;

            virtual void start();

            virtual void stop();

            virtual void restart() {
                this->stop();
                this->start();
            }

        protected:
            virtual void execute() = 0;
            virtual void shouldStop() = 0;
        protected:
            std::thread m_workThread;
            enum class Status:int{
                Stopped,
                Starting,
                Started,
                Stopping,
            };
            std::atomic<Status> m_status;
            std::mutex m_statusMutex;
            std::condition_variable m_statusCV;
        };
    }
}

#endif //KAKAIMCLUSTER_KIMMODULE_H
