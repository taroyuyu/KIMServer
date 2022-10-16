//
// Created by Kakawater on 2018/1/2.
//

#include "KIMModule.h"

namespace kakaIM {
    namespace common {
        bool KIMModule::init(){
            return true;
        }
        void KIMModule::start() {
            std::unique_lock<std::mutex> lock(this->m_statusMutex);
            if (Status::Stopped != this->m_status) {
                return;
            }
            this->m_status = Status::Starting;
            this->m_workThread = std::move(std::thread([this]() {
                this->execute();
            }));
            this->m_statusCV.wait(lock, [this]() {
                return Status::Started != this->m_status;
            });
        }

        void KIMModule::stop() {
            std::unique_lock<std::mutex> lock(this->m_statusMutex);
            if (Status::Started != this->m_status) {
                return;
            }
            this->m_status = Status::Stopping;
            this->shouldStop();
            this->m_statusCV.wait(lock, [this]() {
                return Status::Stopped != this->m_status;
            });
        }

        void KIMModule::addMessage(std::unique_ptr<::google::protobuf::Message> message,
                                   const std::string connectionIdentifier) {
            if (!message) {
                return;
            }

            this->mTaskQueue.push(std::move(std::make_pair(std::move(message), connectionIdentifier));
        }

        void KIMModule::setDBConfig(const KIMDBConfig &dbConfig) {
            this->dbConfig = dbConfig;
        }
    }
}
