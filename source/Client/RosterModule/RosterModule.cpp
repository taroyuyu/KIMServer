//
// Created by Kakawater on 2018/2/1.
//

#include <Client/RosterModule/RosterModule.h>
#include <Common/proto/KakaIMMessage.pb.h>
namespace kakaIM {
    namespace client {
        void RosterModule::setSessionModule(SessionModule &sessionModule){
            this->sessionModule = &sessionModule;
        }

	std::list<std::string> RosterModule::messageTypes(){
            std::list<std::string> messageTypeList;
            messageTypeList.push_back(Node::UserVCardResponseMessage::default_instance().GetTypeName());
            messageTypeList.push_back(Node::UpdateUserVCardMessageResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::FriendListResponseMessage::default_instance().GetTypeName());
            messageTypeList.push_back(Node::BuildingRelationshipRequestMessage::default_instance().GetTypeName());
            messageTypeList.push_back(Node::BuildingRelationshipAnswerMessage::default_instance().GetTypeName());
            messageTypeList.push_back(Node::DestoryingRelationshipResponseMessage::default_instance().GetTypeName());
            return messageTypeList;
        }

        void
        RosterModule::handleMessage(const ::google::protobuf::Message &message, SessionModule &sessionModule){
            const std::string messageType = message.GetTypeName();
            if(messageType == Node::UserVCardResponseMessage::default_instance().GetTypeName()){
                const Node::UserVCardResponseMessage & userVCardResponseMessage = *((const Node::UserVCardResponseMessage*)&message);
                this->handleUserVCardResponseMessage(userVCardResponseMessage);
            }else if(messageType == Node::UpdateUserVCardMessageResponse::default_instance().GetTypeName()){
                const Node::UpdateUserVCardMessageResponse & updateUserVCardMessageResponse = *((const Node::UpdateUserVCardMessageResponse*)&message);
                this->handleUpdateUserVCardMessageResponse(updateUserVCardMessageResponse);
            }else if(messageType == Node::FriendListResponseMessage::default_instance().GetTypeName()){
                const Node::FriendListResponseMessage & friendListResponseMessage = *((const Node::FriendListResponseMessage*)&message);
                this->handleFriendListResponseMessage(friendListResponseMessage);
            }else if(messageType == Node::BuildingRelationshipRequestMessage::default_instance().GetTypeName()){
                const Node::BuildingRelationshipRequestMessage & buildingRelationshipRequestMessage = *((const Node::BuildingRelationshipRequestMessage*)&message);
                this->handleBuildingRelationshipRequestMessage(buildingRelationshipRequestMessage);
            }else if(messageType == Node::BuildingRelationshipAnswerMessage::default_instance().GetTypeName()){
                const Node::BuildingRelationshipAnswerMessage & buildingRelationshipAnswerMessage = *((const Node::BuildingRelationshipAnswerMessage*)&message);
                this->handleBuildingRelationshipAnswerMessage(buildingRelationshipAnswerMessage);
            }else if(messageType == Node::DestoryingRelationshipResponseMessage::default_instance().GetTypeName()){
                const Node::DestoryingRelationshipResponseMessage & destoryingRelationshipResponseMessage = *((const Node::DestoryingRelationshipResponseMessage*)&message);
                this->handleDestoryingRelationshipResponseMessage(destoryingRelationshipResponseMessage);
            }
        }
	void RosterModule::handleUserVCardResponseMessage(const Node::UserVCardResponseMessage &userVCardResponseMessage){
            UserVCard userVCard(userVCardResponseMessage.userid());
            userVCard.setUserNickName(userVCardResponseMessage.nickname());
            userVCard.setUserMood(userVCardResponseMessage.mood());
            switch (userVCardResponseMessage.gender()){
                case Node::Unkown:{
                    userVCard.setUserGender(UserVCard::UserGenderType_Unkown);
                }
                    break;
                case Node::Male:{
                    userVCard.setUserGender(UserVCard::UserGenderType_Male);
                }
                    break;
                case Node::Female:{
                    userVCard.setUserGender(UserVCard::UserGenderType_Female);
                }
                    break;
                default:{

                }
            }
            userVCard.setUserAvator(userVCardResponseMessage.avator());
            if(auto rosterDB = this->mRosterDBPtr.lock()){
                rosterDB->updateUserVcard(userVCard);
            }
            auto callBackIt = std::find_if(this->fetchUserVCardCallbackSet.begin(),this->fetchUserVCardCallbackSet.end(),[&userVCard](const std::pair<std::string,std::pair<FetchUserVCardSuccessCallback,FetchUserVCardFailureCallback>> &callbackPair)-> bool{
                return callbackPair.first == userVCard.getUserAccount();
            });
            if(callBackIt != this->fetchUserVCardCallbackSet.end()){
                callBackIt->second.first(*this,userVCard);
                this->fetchUserVCardCallbackSet.erase(callBackIt);
            }
        }

        void RosterModule::handleUpdateUserVCardMessageResponse(const Node::UpdateUserVCardMessageResponse &updateUserVCardMessageResponse) {
            switch (updateUserVCardMessageResponse.state()){
                case Node::UpdateUserVCardMessageResponse_UpdateUserVCardStateType_Success:{
                    if(this->updateCurrentUserVCardSuccessCallback){
                        this->updateCurrentUserVCardSuccessCallback(*this);
                        this->updateCurrentUserVCardSuccessCallback = nullptr;
                    }
                }
                    break;
                case Node::UpdateUserVCardMessageResponse_UpdateUserVCardStateType_Failure:{
                    if(this->updateCurrentUserVCardFailureCallback){
                        this->updateCurrentUserVCardFailureCallback(*this,UpdateUserVCardFailureType_UserVCardIllegal);
                        this->updateCurrentUserVCardSuccessCallback = nullptr;
                    }
                }
                default:{

                }
            }
        }

        void RosterModule::handleFriendListResponseMessage(const Node::FriendListResponseMessage &friendListResponseMessage) {
            std::list<User> friendList;
            for (int i = 0;i < friendListResponseMessage.friend_().size(); ++i) {
                auto friendListItem = friendListResponseMessage.friend_().Get(i);
                friendList.push_back(friendListItem.friendaccount());
            }
            if(this->fetchFriendListSuccessCallback){
                this->fetchFriendListSuccessCallback(*this,friendList);
                this->fetchFriendListSuccessCallback = nullptr;
            }
        }
        void RosterModule::handleBuildingRelationshipRequestMessage(const Node::BuildingRelationshipRequestMessage & buildingRelationshipRequestMessage){
            std::string sponsorAccount = buildingRelationshipRequestMessage.sponsoraccount();
            std::string targetAccount = buildingRelationshipRequestMessage.targetaccount();
            std::string introducation = buildingRelationshipRequestMessage.introduction();
            uint64_t applicantId = buildingRelationshipRequestMessage.applicant_id();
            if(auto friendRelationDelegate = this->mFriendRelationDelegatePtr.lock()){
                FriendRequest request(sponsorAccount,targetAccount);
                request.setRequestMessage(introducation);
                request.setApplicantId(applicantId);
                friendRelationDelegate->didReceiveFriendRequest(request);
            }
        }
        void RosterModule::handleBuildingRelationshipAnswerMessage(const Node::BuildingRelationshipAnswerMessage & buildingRelationshipAnswerMessage){
            std::string sponsorAccount = buildingRelationshipAnswerMessage.sponsoraccount();
            std::string targetAccount = buildingRelationshipAnswerMessage.targetaccount();
            uint64_t applicationId = buildingRelationshipAnswerMessage.applicant_id();
            FriendRequestAnswerType answerType = FriendRequestAnswerType_Ignore;
            switch (buildingRelationshipAnswerMessage.answer()){
                case Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Accept:{
                    answerType = FriendRequestAnswerType_Accept;
                }
                    break;
                case Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Reject:{
                    answerType = FriendRequesrAnswerType_Reject;
                }
                    break;
                default:{

                }
            }
            if(auto friendRelationDelegate = this->mFriendRelationDelegatePtr.lock()){
                FriendRequestAnswer answer(sponsorAccount,targetAccount,applicationId,answerType);
                friendRelationDelegate->didReceiveFriendRequestAnswer(answer);
            }
        }
        void RosterModule::handleDestoryingRelationshipResponseMessage(const Node::DestoryingRelationshipResponseMessage & destoryingRelationshipResponseMessage){
            std::string sponsorAccount = destoryingRelationshipResponseMessage.sponsoraccount();
            std::string targetAccount = destoryingRelationshipResponseMessage.targetaccount();
            auto response = destoryingRelationshipResponseMessage.response();
            if(sponsorAccount == this->currentUserAccount){
                if(Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_Success == response){
                    if(this->deleteFriendSuccessCallback){
                        this->deleteFriendSuccessCallback(*this);
                    }
                }else if(Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_IllegalOperation){
                    if(this->deleteFriendFailureCallback){
                        this->deleteFriendFailureCallback(*this,DeleteFriendFailureType_FriendRelationNotExitBefore);
                    }
                } else if(Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_ServerInteralError){
                    if(this->deleteFriendFailureCallback){
                        this->deleteFriendFailureCallback(*this,DeleteFriendFailureType_ServerInteralError);
                    }
                }
            }else if(targetAccount == this->currentUserAccount){
                if(Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_Success == response){
                    //1.更新好友列表-Todo
                    //2.通知Delegate
                    if(auto friendRelationDelegate = this->mFriendRelationDelegatePtr.lock()){
                        friendRelationDelegate->didFriendShipRemoveByUser(User(sponsorAccount));
                    }

                }
            }
        }

        void RosterModule::fetchUserVCard(User & user,FetchUserVCardSuccessCallback successCallback,FetchUserVCardFailureCallback failureCallback){
	    auto callBackIt = std::find_if(this->fetchUserVCardCallbackSet.begin(),this->fetchUserVCardCallbackSet.end(),[&user](const std::pair<std::string,std::pair<RosterModule::FetchUserVCardSuccessCallback,RosterModule::FetchUserVCardFailureCallback>> &callbackPair)-> bool{
                return callbackPair.first == user.getUserAccount();
            });
            if(callBackIt != this->fetchUserVCardCallbackSet.end()){
                this->fetchUserVCardCallbackSet.erase(callBackIt);
            }
            this->fetchUserVCardCallbackSet.insert(std::make_pair(user.getUserAccount(),std::make_pair(successCallback,failureCallback)));
            Node::FetchUserVCardMessage fetchUserVCardMessage;
            fetchUserVCardMessage.set_userid(user.getUserAccount());
            this->sessionModule->sendMessage(*((const ::google::protobuf::Message*)&fetchUserVCardMessage));
        }
        void RosterModule::updateCurrentUserVCard(const UserVCard & userVCard,UpdateCurrentUserVCardSuccessCallback successCallback,UpdateCurrentUserVCardFailureCallback failureCallback){
            this->updateCurrentUserVCardSuccessCallback = successCallback;
            this->updateCurrentUserVCardFailureCallback = failureCallback;
            Node::UpdateUserVCardMessage updateUserVCardMessage;
            updateUserVCardMessage.set_nickname(userVCard.getUserNickName());
            switch (userVCard.getUserGender()){
                case UserVCard::UserGenderType_Unkown:{
                    updateUserVCardMessage.set_gender(Node::Unkown);
                }
                    break;
                case UserVCard::UserGenderType_Male:{
                    updateUserVCardMessage.set_gender(Node::Male);
                }
                    break;
                case UserVCard::UserGenderType_Female:{
                    updateUserVCardMessage.set_gender(Node::Female);
                }
                    break;
            }
            updateUserVCardMessage.set_mood(userVCard.getUserMood());
            updateUserVCardMessage.set_avator(userVCard.getUserAvator());
	    this->sessionModule->sendMessage(updateUserVCardMessage);
        }

	        void RosterModule::fetchFriendList(FetchFriendListSuccessCallback successCallback,FetchFriendListFailureCallback failureCallback){
            this->fetchFriendListSuccessCallback = successCallback;
            this->fetchFriendListFailureCallback = failureCallback;
            Node::FriendListRequestMessage friendListRequestMessage;
            this->sessionModule->sendMessage(friendListRequestMessage);
        }

        void RosterModule::sendFriendRequest(const FriendRequest request){
            Node::BuildingRelationshipRequestMessage relationshipRequestMessage;
            relationshipRequestMessage.set_sponsoraccount(request.getSponsorAccount());
            relationshipRequestMessage.set_targetaccount(request.getTargetAccout());
            relationshipRequestMessage.set_introduction(request.getRequestMessage());
            this->sessionModule->sendMessage(relationshipRequestMessage);
        }
        void RosterModule::andswerToFriendRequest(const FriendRequestAnswer answer){
            if(FriendRequestAnswerType_Ignore == answer.getAnswerType()){

            }else{
                Node::BuildingRelationshipAnswerMessage relationshipAnswerMessage;
                relationshipAnswerMessage.set_sponsoraccount(answer.getSponsorAccount());
                relationshipAnswerMessage.set_targetaccount(answer.getTargetAccout());
                relationshipAnswerMessage.set_applicant_id(answer.getApplicationId());
                switch (answer.getAnswerType()){
                    case FriendRequestAnswerType_Accept:{
                        relationshipAnswerMessage.set_answer(Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Accept);
                    }
                        break;
                    case FriendRequesrAnswerType_Reject:{
                        relationshipAnswerMessage.set_answer(Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Reject);
                    }
                        break;
                    default:{

                    }
                }
                this->sessionModule->sendMessage(relationshipAnswerMessage);
            }
        }

        void RosterModule::deleteFriend(const User user,DeleteFriendSuccessCallback successCallback,DeleteFriendFailureCallback failureCallback){
            this->deleteFriendSuccessCallback = successCallback;
            this->deleteFriendFailureCallback = failureCallback;
            Node::DestroyingRelationshipRequestMessage destroyingRelationshipRequestMessage;
            destroyingRelationshipRequestMessage.set_sponsoraccount(this->currentUserAccount);
            destroyingRelationshipRequestMessage.set_targetaccount(user.getUserAccount());
            this->sessionModule->sendMessage(destroyingRelationshipRequestMessage);
        }
    }
}
