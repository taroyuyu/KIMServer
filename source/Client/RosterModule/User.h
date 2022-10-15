//
// Created by Kakawater on 2018/2/17.
//

#ifndef KAKAIMCLUSTER_USER_H
#define KAKAIMCLUSTER_USER_H

#include <string>

namespace kakaIM {
    namespace client {
        class UserVCard{
        public:
            enum UserGenderType
            {
                UserGenderType_Male = 0,//男
                UserGenderType_Female,//女
                UserGenderType_Unkown,//未知
            };
            UserVCard(const std::string userAccount):userAccount(userAccount),gender(UserGenderType_Unkown){

            }
            void setUserAccount(const std::string userAccount){
                this->userAccount = userAccount;
            }
            const std::string getUserAccount()const{
                return this->userAccount;
            }
            void setUserNickName(const std::string userNickName){
                this->nickname = userNickName;
            }
            const std::string getUserNickName()const{
                return this->nickname;
            }
            void setUserMood(const std::string userMood){
                this->mood = userMood;
            }
            const std::string getUserMood()const{
                return this->mood;
            }
            void setUserGender(const UserGenderType genderType){
                this->gender = genderType;
            }
            const UserGenderType getUserGender()const{
                return this->gender;
            }
            void setUserAvator(const std::string & userAvator){
                this->avator = userAvator;
            }
            const std::string getUserAvator()const{
                return this->avator;
            }
        private:
            std::string userAccount;
            std::string nickname;
            std::string mood;
            std::string avator;
            UserGenderType gender;
        };
        class User {
        public:
            enum UserOnlineState{
                UserOnlineState_Online,
                UserOnlineState_Offline,
                UserOnlineState_Invisible,
            };
            User(std::string userAccount):userAccount(userAccount),userOnlineState(UserOnlineState_Offline){

            }
            const std::string getUserAccount()const{
                return this->userAccount;
            }
            void setUserOnlineState(const UserOnlineState userOnlineState){
                this->userOnlineState = userOnlineState;
            }
            const UserOnlineState getUserOnlineState()const{
                return this->userOnlineState;
            }
        private:
            std::string userAccount;
            enum UserOnlineState userOnlineState;
        };

    }
}

#endif //KAKAIMCLUSTER_USER_H

