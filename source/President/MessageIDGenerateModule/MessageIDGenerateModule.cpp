//
// Created by Kakawater on 2018/1/28.
//

#include <zconf.h>
#include <sys/time.h>
#include <cstdlib>
#include <bitset>
#include <typeinfo>
#include <limits>
#include <typeinfo>
#include <President/MessageIDGenerateModule/MessageIDGenerateModule.h>
#include <President/Log/log.h>

namespace kakaIM {
    namespace president {
        uint64_t generateStamp();

        uint64_t waitNextMs(uint64_t lastStamp);

        MessageIDGenerateModule::MessageIDGenerateModule() : KIMPresidentModule(MessageIDGenerateModuleLogger){
            this->mMessageTypeSet.insert(RequestMessageIDMessage::default_instance().GetTypeName());
        }

        MessageIDGenerateModule::~MessageIDGenerateModule() {
            this->messageIDMutexSet.clear();

        }

        const std::unordered_set<std::string> & MessageIDGenerateModule::messageTypes(){
            return this->mMessageTypeSet;
        }

        void MessageIDGenerateModule::dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task){
            if (task.first->GetTypeName() == RequestMessageIDMessage::default_instance().GetTypeName()){
                this->handleRequestMessageIDMessage(dynamic_cast<RequestMessageIDMessage &>(*(task.first)), task.second);
            }
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

