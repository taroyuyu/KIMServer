//
// Created by Kakawater on 2018/3/8.
//

#ifndef KAKAIMCLUSTER_GROUPCHATMMESSAGE_H
#define KAKAIMCLUSTER_GROUPCHATMMESSAGE_H
#include <string>
namespace kakaIM {
    namespace client {
        class GroupChatMessage
        {
        public:
	    GroupChatMessage(std::string groupId,std::string senderId,std::string content):
                    groupId(groupId),senderId(senderId),content(content)
            {

            }
            std::string getGroupId()const{
                return this->groupId;
            }
            std::string getSenderId()const{
                return this->senderId;
            }
            std::string getContent()const{
                return this->content;
            }
            uint64_t getMessageID()const{
                return this->messageID;
            }
            void setMessageID(const uint64_t messageID){
                this->messageID = messageID;
            }
        private:
            std::string groupId;
            std::string senderId;
            std::string content;
            uint64_t messageID;
        };
    }
}
#endif //KAKAIMCLUSTER_GROUPCHATMMESSAGE_H
