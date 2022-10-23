//
// Created by Kakawater on 2018/1/28.
//

#ifndef KAKAIMCLUSTER_MESSAGEIDGENERATEMODULE_H
#define KAKAIMCLUSTER_MESSAGEIDGENERATEMODULE_H

#include <President/KIMPresidentModule/KIMPresidentModule.h>
#include <map>
#include <Common/util/Date.h>
#include <Common/proto/MessageCluster.pb.h>
#include <functional>

namespace kakaIM {
    namespace president {
        class MessageIDGenerateModule : public KIMPresidentModule {
        public:
            MessageIDGenerateModule();

            ~MessageIDGenerateModule();
            virtual const std::unordered_set<std::string> messageTypes() override;
        protected:
            virtual void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task) override;
            std::unordered_map<std::string,std::function<void(std::unique_ptr<::google::protobuf::Message>, const std::string)>> mMessageHandlerSet;
        private:

            ConcurrentLinkedQueue<std::pair<std::shared_ptr<const RequestMessageIDMessage>, const std::string>> mTaskQueue;

            void handleRequestMessageIDMessage(const RequestMessageIDMessage &message,
                                               const std::string connectionIdentifier);

            /**
             * messageIDDB的key-value形式为:userAccount - <lastTimestamp,sequence>
             */
            std::map<std::string, std::pair<uint64_t, uint32_t>> messageIDDB;
            std::map<std::string, std::unique_ptr<std::mutex>> messageIDMutexSet;
            std::mutex messageIDMutexSetMutex;
        };
    }
}


#endif //KAKAIMCLUSTER_MESSAGEIDGENERATEMODULE_H


