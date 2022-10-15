//
// Created by Kakawater on 2018/3/8.
//

#ifndef KAKAIMCLUSTER_CHATGROUP_H
#define KAKAIMCLUSTER_CHATGROUP_H

#include <string>

namespace kakaIM {
    namespace client {
        class ChatGroup{
        public:
            ChatGroup(std::string groupID,std::string groupName):
                    groupID(groupID),groupName(groupName){

            }
            void setName(std::string groupName){
                this->groupName = groupName;
            }
            void setDescription(std::string groupDescription){
                this->groupDescription = groupDescription;
            }
            std::string getID()const{
                return this->groupID;
            }
            std::string getName()const{
                return this->groupName;
            }
            std::string getDescription()const{
                return this->groupDescription;
            }
        private:
            std::string groupID;
            std::string groupName;
            std::string groupDescription;
        };
    }
}
#endif //KAKAIMCLUSTER_CHATGROUP_H

