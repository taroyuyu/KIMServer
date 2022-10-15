//
// Created by Kakawater on 2018/2/1.
//

#include <Client/SingleChatModule/SingleChatModule.h>
#include <Common/proto/KakaIMMessage.pb.h>
namespace kakaIM {
    namespace client {

        void SingleChatModule::setSessionModule(SessionModule &sessionModule){
            this->sessionModule = &sessionModule;
        }

        std::list<std::string> SingleChatModule::messageTypes(){
            std::list<std::string> messageTypeList;
            messageTypeList.push_back(Node::ChatMessage::default_instance().GetTypeName());
            return messageTypeList;
        }

        void SingleChatModule::handleMessage(const ::google::protobuf::Message &message, SessionModule &sessionModule){
            const std::string messageType = message.GetTypeName();
            if(messageType == Node::ChatMessage::default_instance().GetTypeName()){
                const Node::ChatMessage & chatMessage = *((const Node::ChatMessage*)&message);
                this->handleChatMessage(chatMessage);
            }
        }

        void SingleChatModule::handleChatMessage(const Node::ChatMessage & chatMessage){
            ChatMessage message(chatMessage.senderaccount(),chatMessage.receiveraccount(),chatMessage.content());
            message.setTimestamp(chatMessage.timestamp());
            message.setMessageID(chatMessage.messageid());
            if(auto chatMessageDb = this->mSingleChatMessageDBPtr.lock()){
                chatMessageDb->storeChatMessage(message);
            }

            if(auto delegate = this->mSingleCahtMessageDelegatePtr.lock()){
                delegate->didReceiveSingleChatMessage(message);
            }
        }

        void SingleChatModule::sendMessage(const ChatMessage & chatMessage){
            Node::ChatMessage message;
            message.set_senderaccount(chatMessage.getSenderAccount());
            message.set_receiveraccount(chatMessage.getReceiverAccount());
            message.set_content(chatMessage.getContent());
            message.set_timestamp(chatMessage.getTimestamp().toString());
            this->sessionModule->sendMessage(message);
        }
    }
}
