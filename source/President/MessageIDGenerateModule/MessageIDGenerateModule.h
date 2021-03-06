//
// Created by taroyuyu on 2018/1/28.
//

#ifndef KAKAIMCLUSTER_MESSAGEIDGENERATEMODULE_H
#define KAKAIMCLUSTER_MESSAGEIDGENERATEMODULE_H

#include <string>
#include <map>
#include <queue>
#include <mutex>
#include <memory>
#include <log4cxx/logger.h>
#include "../../Common/KIMModule.h"
#include "../../Common/util/Date.h"
#include "../../Common/proto/MessageCluster.pb.h"
#include "../Service/ConnectionOperationService.h"
namespace kakaIM {
    namespace president {
        class MessageIDGenerateModule : public common::KIMModule {
        public:
            MessageIDGenerateModule();

            ~MessageIDGenerateModule();

            virtual bool init() override;

            virtual void execute() override;

            void setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr);

            void
            addRequestMessageIDMessage(const RequestMessageIDMessage &message, const std::string connectionIdentifier);

        private:
            int messageEventfd;

            void handleRequestMessageIDMessage(const RequestMessageIDMessage &message,
                                               const std::string connectionIdentifier);

            std::mutex messageQueueMutex;
            std::queue<std::pair<std::shared_ptr<const RequestMessageIDMessage>, const std::string>> messageQueue;

            std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr;

            /**
             * messageIDDB的key-value形式为:userAccount - <lastTimestamp,sequence>
             */
            std::map<std::string, std::pair<uint64_t, uint32_t>> messageIDDB;
            std::map<std::string, std::unique_ptr<std::mutex>> messageIDMutexSet;
            std::mutex messageIDMutexSetMutex;

	    log4cxx::LoggerPtr logger;
        };
    }
}


#endif //KAKAIMCLUSTER_MESSAGEIDGENERATEMODULE_H


