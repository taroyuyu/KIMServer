//
// Created by Kakawater on 2018/2/1.
//

#ifndef KAKAIMCLUSTER_SINGLECHATMODULE_H
#define KAKAIMCLUSTER_SINGLECHATMODULE_H

#include "../ServiceModule/ServiceModule.h"
#include "SingleChatMessageDB.h"
#include "SingleChatMessageDelegate.h"
#include "ChatMessage.h"
namespace kakaIM {
    namespace Node{
        class ChatMessage;
    }
    namespace client {
        class SingleChatModule : public ServiceModule {
        public:
            SingleChatModule():sessionModule(nullptr){

            }
            void setSingleChatMessageDB(std::weak_ptr<SingleChatMessageDB> singleChatMessageDB){
                this->mSingleChatMessageDBPtr = singleChatMessageDB;
            }
            void setDelegate(std::weak_ptr<SingleChatMessageDelegate> delegate){
                this->mSingleCahtMessageDelegatePtr = delegate;
            }
            void sendMessage(const ChatMessage & chatMessage);
        protected:
            virtual void setSessionModule(SessionModule &sessionModule) override;

            virtual std::list<std::string> messageTypes() override;

            virtual void handleMessage(const ::google::protobuf::Message &message, SessionModule &sessionModule) override;

        private:
            SessionModule *sessionModule;
            std::weak_ptr<SingleChatMessageDB> mSingleChatMessageDBPtr;
            std::weak_ptr<SingleChatMessageDelegate> mSingleCahtMessageDelegatePtr;
            void handleChatMessage(const Node::ChatMessage & chatMessage);
        };
    }
}


#endif //KAKAIMCLUSTER_SINGLECHATMODULE_H

