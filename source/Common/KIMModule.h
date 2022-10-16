//
// Created by Kakawater on 2018/1/1.
//

#ifndef KAKAIMCLUSTER_KIMMODULE_H
#define KAKAIMCLUSTER_KIMMODULE_H

#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <google/protobuf/message.h>
#include "ConcurrentQueue/ConcurrentLinkedQueue.h"
#include "KIMDBConfig.h"

namespace kakaIM {
    namespace common {
        class KIMModule {
        public:
            KIMModule() : m_status(Status::Stopped) {
            }

            virtual bool init();

            virtual void start();

            virtual void stop();

            virtual void restart() {
                this->stop();
                this->start();
            }

            virtual void
            addMessage(std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier);

            virtual void setDBConfig(const KIMDBConfig &dbConfig);

        protected:
            virtual void execute() = 0;

            virtual void shouldStop() = 0;

            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> mTaskQueue;
            KIMDBConfig dbConfig;
        protected:
            std::thread m_workThread;
            enum class Status : int {
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
