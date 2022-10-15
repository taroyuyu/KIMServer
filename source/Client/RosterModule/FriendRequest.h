//
// Created by Kakawater on 2018/2/26.
//

#ifndef KAKAIMCLUSTER_FRIENDREQUEST_H
#define KAKAIMCLUSTER_FRIENDREQUEST_H
namespace kakaIM {
    namespace client {

        enum FriendRequestAnswerType
        {
            FriendRequestAnswerType_Accept,//接收好友请求
            FriendRequesrAnswerType_Reject,//拒绝好友请求
            FriendRequestAnswerType_Ignore,//忽略好友请求
        };
        class FriendRequest {
        public:
            FriendRequest(std::string sponsorAccount,std::string targetAccout):sponsorAccount(sponsorAccount),targetAccout(targetAccout),applicant_id(0){

            }
            void setRequestMessage(const std::string requestMessage){
                this->requestMessage = requestMessage;
            }
            void setApplicantId(const uint64_t applicantId){
                this->applicant_id = applicantId;
            }
            const std::string getSponsorAccount()const{
                return this->sponsorAccount;
            }
            const std::string getTargetAccout()const{
                return this->targetAccout;
            }
            const std::string getRequestMessage()const{
                return this->requestMessage;
            }
            const uint64_t getApplicanId()const{
                return this->applicant_id;
            }
        private:
            std::string sponsorAccount;
            std::string targetAccout;
            std::string requestMessage;
            uint64_t applicant_id;
        };

        class FriendRequestAnswer{
        public:
            FriendRequestAnswer(std::string sponsorAccount,std::string targetAccout,uint64_t applicationId,FriendRequestAnswerType answerType):sponsorAccount(sponsorAccount),targetAccout(targetAccout),answerType(answerType),application_id(applicationId){

            }
            void setReason(const std::string reason){
                this->reason = reason;
            }
            const std::string getSponsorAccount()const{
                return this->sponsorAccount;
            }
            const std::string getTargetAccout()const{
                return this->targetAccout;
            }
            const std::string getReason()const{
                return this->reason;
            }
            const FriendRequestAnswerType getAnswerType()const {
                return this->answerType;
            }
            const uint64_t getApplicationId()const{
                return this->application_id;
            }
        private:
            std::string sponsorAccount;
            std::string targetAccout;
            std::string reason;
            enum FriendRequestAnswerType answerType;
            uint64_t application_id;
        };
    }
}
#endif //KAKAIMCLUSTER_FRIENDREQUEST_H

