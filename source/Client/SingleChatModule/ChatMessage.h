//
// Created by Kakawater on 2018/2/28.
//

#ifndef KAKAIMCLUSTER_CHATMESSAGE_H
#define KAKAIMCLUSTER_CHATMESSAGE_H

#include <Common/util/Date.h>
#include <utility>
namespace kakaIM {
    namespace client {
        class ChatMessage{
        public:
            ChatMessage(std::string senderAccount,std::string receiverAccount,std::string content):senderAccount(senderAccount),receiverAccount(receiverAccount),content(content),timestamp(kaka::Date::getCurrentDate()),messageID(0){

            }
            void setContent(const std::string content){
                this->content = content;
            }
            void setTimestamp(const kaka::Date date){
                this->timestamp = date;
            }
            void setMessageID(const uint64_t messageID){
                this->messageID = messageID;
            }

            std::string getSenderAccount()const {
                return this->senderAccount;
            }
            std::string getReceiverAccount()const{
                return this->receiverAccount;
            }
            std::string getContent()const{
                return this->content;
            }
            kaka::Date getTimestamp()const{
                return this->timestamp;
            }
	    uint64_t getMessageID()const{
                return this->messageID;
            }
        private:
            std::string senderAccount;
            std::string receiverAccount;
            std::string content;
            kaka::Date timestamp;
            uint64_t messageID;
        };
    }
}

#endif //KAKAIMCLUSTER_CHATMESSAGE_H

