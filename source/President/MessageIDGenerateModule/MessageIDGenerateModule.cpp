//
// Created by Kakawater on 2018/1/28.
//

#include <zconf.h>
#include <sys/eventfd.h>
#include <sys/time.h>
#include <cstdlib>
#include <bitset>
#include <typeinfo>
#include <limits>
#include <typeinfo>
#include "MessageIDGenerateModule.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace president {
        uint64_t generateStamp();

        uint64_t waitNextMs(uint64_t lastStamp);

        MessageIDGenerateModule::MessageIDGenerateModule() : messageEventfd(-1) {
            this->logger = log4cxx::Logger::getLogger(MessageIDGenerateModuleLogger);
        }

        MessageIDGenerateModule::~MessageIDGenerateModule() {
            while (false == this->messageQueue.empty()) {
                this->messageQueue.pop();
            }

            this->messageIDMutexSet.clear();

        }

        bool MessageIDGenerateModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }
            return true;
        }

        void MessageIDGenerateModule::execute() {
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (not this->m_needStop) {
                if (auto task = this->mTaskQueue.try_pop()) {
                    this->dispatchMessage(*task);
                } else {
                    std::this_thread::yield();
                }
            }

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }

        void MessageIDGenerateModule::dispatchMessage(
                std::pair<std::shared_ptr<const RequestMessageIDMessage>, const std::string> &task) {
            this->handleRequestMessageIDMessage(*(task.first.get()), task.second);
        }

        void MessageIDGenerateModule::shouldStop() {
            this->m_needStop = true;
        }

        void MessageIDGenerateModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void MessageIDGenerateModule::addRequestMessageIDMessage(std::unique_ptr<RequestMessageIDMessage> message,
                                                                 const std::string connectionIdentifier) {
            if (!message) {
                return;
            }
            //添加到队列中
            this->mTaskQueue.push(std::move(std::make_pair(std::move(message),connectionIdentifier)));
        }

        void MessageIDGenerateModule::handleRequestMessageIDMessage(const RequestMessageIDMessage &message,
                                                                    const std::string connectionIdentifier) {

            std::string userAccount = message.useraccount();
            auto messageIDMutexIt = this->messageIDMutexSet.find(userAccount);
            if (messageIDMutexIt == this->messageIDMutexSet.end()) {
                std::unique_lock<std::mutex> lock(this->messageIDMutexSetMutex);
                this->messageIDMutexSet.emplace(userAccount, std::move(std::unique_ptr<std::mutex>(new std::mutex())));
                messageIDMutexIt = this->messageIDMutexSet.find(userAccount);
            }

            std::unique_lock<std::mutex> messageIDlock(*(messageIDMutexIt->second));

            auto messageIDPairIt = this->messageIDDB.find(userAccount);

            uint64_t lastTimestamp = 0;
            uint32_t sequence = 0;
            if (messageIDPairIt != this->messageIDDB.end()) {
                lastTimestamp = messageIDPairIt->second.first;
                sequence = messageIDPairIt->second.second;
                this->messageIDDB.erase(messageIDPairIt);
            }

            uint64_t currentTimestamp = generateStamp();
            // 如果当前时间小于上一次ID生成的时间戳，说明系统时钟回退过这个时候应当抛出异常
            if (currentTimestamp < lastTimestamp) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                << "clock moved backwards.  Refusing to generate id for "
                                                                << lastTimestamp - currentTimestamp << " milliseconds");
                //抛出异常
            }

            if (currentTimestamp == lastTimestamp) {
                // 如果是同一时间生成的，则进行毫秒内序列递增
                ++(sequence);
                if (0x800000 == sequence) {
                    // 毫秒内序列溢出, 阻塞到下一个毫秒,获得新的时间戳
                    currentTimestamp = waitNextMs(lastTimestamp);
                    sequence = 0;
                }
            } else {
                sequence = 0;
            }
            this->messageIDDB.emplace(userAccount, std::make_pair(currentTimestamp, sequence));
            messageIDlock.unlock();

            std::bitset<64> messageID(currentTimestamp);
            messageID <<= 23;
            messageID.reset(messageID.size() - 1);//最高位置0,避免超出postgreSQL的bigserial的最大数值

            messageID |= (std::bitset<64>(sequence) & std::bitset<64>(0x7FFFFF));;


            ResponseMessageIDMessage responseMessageIDMessage;
            responseMessageIDMessage.set_useraccount(userAccount);
            responseMessageIDMessage.set_messageid(messageID.to_ullong());
            responseMessageIDMessage.set_requestid(message.requestid());
            assert(responseMessageIDMessage.IsInitialized());
            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                         responseMessageIDMessage);
            }
        }

        uint64_t generateStamp() {
            timeval tv;
            gettimeofday(&tv, 0);
            return (uint64_t) tv.tv_sec * 1000 + (uint64_t) tv.tv_usec / 1000;
        }

        uint64_t waitNextMs(uint64_t lastStamp) {
            uint64_t cur = 0;
            do {
                cur = generateStamp();
            } while (cur <= lastStamp);
            return cur;
        }
    }
}

