//
// Created by Kakawater on 2018/3/8.
//

#include "GroupChatModule.h"


namespace kakaIM {
    namespace client {

        void GroupChatModule::setSessionModule(SessionModule & sessionModule)
        {
            this->sessionModule = &sessionModule;
        }
        std::list<std::string> GroupChatModule::messageTypes()
        {
            std::list<std::string> messageTypeList;
            messageTypeList.push_back(Node::ChatGroupCreateResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::ChatGroupDisbandResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::ChatGroupJoinResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::ChatGroupQuitResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::UpdateChatGroupInfoResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::FetchChatGroupInfoResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::FetchChatGroupMemberListResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::FetchChatGroupListResponse::default_instance().GetTypeName());
            messageTypeList.push_back(Node::GroupChatMessage::default_instance().GetTypeName());
            return messageTypeList;
        }

        void GroupChatModule::handleMessage(const ::google::protobuf::Message & message,SessionModule & sessionModule){
            const std::string messageType = message.GetTypeName();
            if(messageType == Node::ChatGroupCreateResponse::default_instance().GetTypeName()){
                const Node::ChatGroupCreateResponse & chatGroupCreateResponse = *((const Node::ChatGroupCreateResponse*)&message);
                this->handleChatGroupCreateResponseMessage(chatGroupCreateResponse);
            }else if(messageType == Node::ChatGroupDisbandResponse::default_instance().GetTypeName()){
                const Node::ChatGroupDisbandResponse & chatGroupDisbandResponse  = *((const Node::ChatGroupDisbandResponse*)&message);
                this->handleChatGroupDisbandResponseMessage(chatGroupDisbandResponse);
            }else if(messageType == Node::ChatGroupJoinResponse::default_instance().GetTypeName()){
                const Node::ChatGroupJoinResponse & chatGroupJoinResponse  = *((const Node::ChatGroupJoinResponse*)&message);
                this->handleChatGroupJoinResponseMessage(chatGroupJoinResponse);
            }else if(messageType == Node::ChatGroupQuitResponse::default_instance().GetTypeName()){
                const Node::ChatGroupQuitResponse & chatGroupQuitResponse  = *((const Node::ChatGroupQuitResponse*)&message);
                this->handleChatGroupQuitResponseMessage(chatGroupQuitResponse);
            }else if(messageType == Node::UpdateChatGroupInfoResponse::default_instance().GetTypeName()){
                const Node::UpdateChatGroupInfoResponse & updateChatGroupInfoResponse = *((const Node::UpdateChatGroupInfoResponse*)&message);
                this->handleUpdateChatGroupInfoResponseMessage(updateChatGroupInfoResponse);
            }else if(messageType == Node::FetchChatGroupInfoResponse::default_instance().GetTypeName()){
                const Node::FetchChatGroupInfoResponse & fetchChatGroupInfoResponse  = *((const Node::FetchChatGroupInfoResponse*)&message);
                this->handleFetchChatGroupInfoResponseMessage(fetchChatGroupInfoResponse);
            }else if(messageType == Node::FetchChatGroupMemberListResponse::default_instance().GetTypeName()){
                const Node::FetchChatGroupMemberListResponse & fetchChatGroupMemberListResponse  = *((const Node::FetchChatGroupMemberListResponse*)&message);
                this->handleFetchChatGroupMemberListResponseMessage(fetchChatGroupMemberListResponse);
            }else if(messageType == Node::FetchChatGroupListResponse::default_instance().GetTypeName()){
                const Node::FetchChatGroupListResponse & fetchChatGroupListResponse  = *((const Node::FetchChatGroupListResponse*)&message);
                this->handleFetchChatGroupListResponseMessage(fetchChatGroupListResponse);
            }else if(messageType == Node::GroupChatMessage::default_instance().GetTypeName()){
                const Node::GroupChatMessage & groupChatMessage = *((const Node::GroupChatMessage*)&message);
                this->handleGroupChatMessage(groupChatMessage);
            }
        }

        void GroupChatModule::handleChatGroupCreateResponseMessage(const Node::ChatGroupCreateResponse & message)
        {
            if(Node::ChatGroupCreateResponse_ChatGroupCreateResponseResult_Success == message.result()){
                if(this->createChatGroupSuccessCallback){
                    this->createChatGroupSuccessCallback(*this,message.groupid());
                    this->createChatGroupSuccessCallback = nullptr;
                }
            }else if(Node::ChatGroupCreateResponse_ChatGroupCreateResponseResult_Failed == message.result()){
                if(this->createChatGroupFailureCallback){
                    this->createChatGroupFailureCallback(*this,CreateChatGroupFailureType_ServerInteralError);
                    this->createChatGroupFailureCallback = nullptr;
                }
            }
        }
        void GroupChatModule::handleChatGroupDisbandResponseMessage(const Node::ChatGroupDisbandResponse & message)
        {
            if(Node::ChatGroupDisbandResponse_ChatGroupDisbandResponseResult_Success == message.result()){
                if(this->disbandChatGroupSuccessCallback){
                    this->disbandChatGroupSuccessCallback(*this);
                    this->disbandChatGroupSuccessCallback = nullptr;
                }
            }else if(Node::ChatGroupDisbandResponse_ChatGroupDisbandResponseResult_Failed == message.result()){
                if(this->disbandChatGroupFailureCallback){
                    this->disbandChatGroupFailureCallback(*this,DisbandChatGroupFailureType_IllegalOperate);
                    this->disbandChatGroupFailureCallback = nullptr;
                }
            }
        }
        void GroupChatModule::handleChatGroupJoinResponseMessage(const kakaIM::Node::ChatGroupJoinResponse & message)
        {
            if(kakaIM::Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_Allow == message.result()){
                if(this->joinChatGroupSuccessCallback){
                    this->joinChatGroupSuccessCallback(*this);
                    this->joinChatGroupSuccessCallback = nullptr;
                }
            }else{
                if(this->joinChatGroupFailureCallback){
                    this->joinChatGroupFailureCallback(*this,JoinChatGroupFailureType_NotAllow);
                    this->joinChatGroupFailureCallback = nullptr;
                }
            }
        }
        void GroupChatModule::handleChatGroupQuitResponseMessage(const Node::ChatGroupQuitResponse & message)
        {
            if(Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Success == message.result()){
                if(this->quitChatGroupSuccessCallback){
                    this->quitChatGroupSuccessCallback(*this);
                    this->quitChatGroupSuccessCallback = nullptr;
                }
            }else if(Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Failed == message.result()){
                if(this->quitChatGroupFailureCallback){
                    this->quitChatGroupFailureCallback(*this,QuitChatGroupFailureType_IllegalOperate);
                    this->quitChatGroupFailureCallback = nullptr;
                }
            }
        }
        void GroupChatModule::handleUpdateChatGroupInfoResponseMessage(const Node::UpdateChatGroupInfoResponse & message)
        {
            if(Node::UpdateChatGroupInfoResponse_UpdateChatGroupInfoResponseResult_Success == message.result()){
                if(this->updateChatGroupInfoSuccessCallback){
                    this->updateChatGroupInfoSuccessCallback(*this);
                    this->updateChatGroupInfoSuccessCallback = nullptr;
                }
            }else if(Node::UpdateChatGroupInfoResponse_UpdateChatGroupInfoResponseResult_Failed == message.result()){
                if(this->updateChatGroupInfoFailureCallback){
                    this->updateChatGroupInfoFailureCallback(*this,UpdateChatGroupInfoFailureType_IllegalOperate);
                }
            }
        }
        void GroupChatModule::handleFetchChatGroupInfoResponseMessage(const Node::FetchChatGroupInfoResponse & message)
        {
            if (Node::FetchChatGroupInfoResponse_FetchChatGroupInfoResponseResult_Success == message.result()){
                if(this->fetchChatGroupInfoSuccessCallback){
                    ChatGroup group(message.groupid(),message.groupname());
                    group.setDescription(message.groupdescrption());
                    this->fetchChatGroupInfoSuccessCallback(*this,group);
                    this->fetchChatGroupInfoSuccessCallback = nullptr;
                }
            }else if(Node::FetchChatGroupInfoResponse_FetchChatGroupInfoResponseResult_Failed == message.result()){

            }
        }
        void GroupChatModule::handleFetchChatGroupMemberListResponseMessage(const Node::FetchChatGroupMemberListResponse & message)
        {
            if(Node::FetchChatGroupMemberListResponse_FetchChatGroupMemberListResponseResult_Success == message.result()){
                if(this->fetchChatGroupMemberListSuccessCallback){
                    std::list<std::pair<std::string,std::string>> memberList;
                    for (int i = 0;i < message.groupmember().size(); ++i) {
                        auto member = message.groupmember().Get(i);
                        memberList.push_back(std::make_pair(member.useraccount(),member.groupnickname()));
                    }
                    this->fetchChatGroupMemberListSuccessCallback(*this,memberList);
                    this->fetchChatGroupMemberListSuccessCallback = nullptr;
                }
            }else if(Node::FetchChatGroupMemberListResponse_FetchChatGroupMemberListResponseResult_Failed){
                if(this->fetchChatGroupMemberListFailureCallback){
                    this->fetchChatGroupMemberListFailureCallback(*this,FetchChatGroupMemberListFailureType_IllegalOperate);
                }
            }
        }
        void GroupChatModule::handleFetchChatGroupListResponseMessage(const Node::FetchChatGroupListResponse & message)
        {
            if(this->fetchCurrentUserChatGroupListSuccessCallback){
                std::list<ChatGroup> groupList;
                for (int i = 0; i < message.group().size(); ++i) {
                    auto group = message.group().Get(i);
                    groupList.push_back(ChatGroup(group.groupid(),group.groupname()));
                }
                this->fetchCurrentUserChatGroupListSuccessCallback(*this,groupList);
                this->fetchCurrentUserChatGroupListSuccessCallback = nullptr;
            }
        }
        void GroupChatModule::handleGroupChatMessage(const Node::GroupChatMessage & message)
        {
            GroupChatMessage groupChatMessage(message.groupid(),message.sender(),message.content());
            if(auto groupChatMessageDelegatePtr = this->mGroupChatMessageDelegatePtr.lock()){
                groupChatMessageDelegatePtr->didReceiveGroupChatMessage(groupChatMessage);
            }
        }

        void GroupChatModule::createChatGroup(const std::string groupName,const std::string groupDescription,CreateChatGroupSuccessCallback successCallback,CreateChatGroupFailureCallback failureCallback){
            Node::ChatGroupCreateRequest chatGroupCreateRequest;
            chatGroupCreateRequest.set_groupname(groupName);
            chatGroupCreateRequest.set_groupdescrption(groupDescription);
            this->createChatGroupSuccessCallback = successCallback;
            this->createChatGroupFailureCallback = failureCallback;
            this->sessionModule->sendMessage(chatGroupCreateRequest);
        }

        void GroupChatModule::disbandChatGroup(const std::string groupId,DisbandChatGroupSuccessCallback successCallback,DisbandChatGroupFailureCallback failureCallback){
            Node::ChatGroupDisbandRequest chatGroupDisbandRequest;
            chatGroupDisbandRequest.set_groupid(groupId);
            chatGroupDisbandRequest.set_operatorid(this->currentUserAccount);
            this->disbandChatGroupSuccessCallback = successCallback;
            this->disbandChatGroupFailureCallback = failureCallback;
            this->sessionModule->sendMessage(chatGroupDisbandRequest);
        }

        void GroupChatModule::joinChatGroup(const std::string groupid,JoinChatGroupSuccessCallback successCallback,JoinChatGroupFailureCallback failureCallback){
            Node::ChatGroupJoinRequest chatGroupJoinRequest;
            chatGroupJoinRequest.set_groupid(groupid);
            chatGroupJoinRequest.set_useraccount(this->currentUserAccount);
            this->joinChatGroupSuccessCallback = successCallback;
            this->joinChatGroupFailureCallback = failureCallback;
            this->sessionModule->sendMessage(chatGroupJoinRequest);
        }

        void GroupChatModule::quitChatGroup(const std::string groupId,QuitChatGroupSuccessCallback successCallback,QuitChatGroupFailureCallback failureCallback){
            Node::ChatGroupQuitRequest chatGroupQuitRequest;
            chatGroupQuitRequest.set_groupid(groupId);
            chatGroupQuitRequest.set_useraccount(this->currentUserAccount);
            this->quitChatGroupSuccessCallback = successCallback;
            this->quitChatGroupFailureCallback = failureCallback;
            this->sessionModule->sendMessage(chatGroupQuitRequest);
        }

        void GroupChatModule::updateChatGroupInfo(const std::string groupId,const std::string groupName,const std::string groupDescription,UpdateChatGroupInfoSuccessCallback successCallback,UpdateChatGroupInfoFailureCallback failureCallback){
            Node::UpdateChatGroupInfoRequest updateChatGroupInfoRequest;
            updateChatGroupInfoRequest.set_groupid(groupId);
            updateChatGroupInfoRequest.set_groupname(groupName);
            updateChatGroupInfoRequest.set_groupdescrption(groupDescription);
            this->updateChatGroupInfoSuccessCallback = successCallback;
            this->updateChatGroupInfoFailureCallback = failureCallback;
            this->sessionModule->sendMessage(updateChatGroupInfoRequest);
        }

        void GroupChatModule::fetchChatGroupInfo(std::string groupID,FetchChatGroupInfoSuccessCallback successCallback,FetchChatGroupInfoFailureCallback failureCallback){
            Node::FetchChatGroupInfoRequest fetchChatGroupInfoRequest;
            fetchChatGroupInfoRequest.set_groupid(groupID);
            this->fetchChatGroupInfoSuccessCallback = successCallback;
            this->fetchChatGroupInfoFailureCallback = failureCallback;
            this->sessionModule->sendMessage(fetchChatGroupInfoRequest);
        }

        void GroupChatModule::fetchChatGroupMemberList(const std::string groupID,FetchChatGroupMemberListSuccessCallback successCallback,FetchChatGroupMemberListFailureCallback failureCallback){
            Node::FetchChatGroupMemberListRequest fetchChatGroupMemberListRequest;
            fetchChatGroupMemberListRequest.set_groupid(groupID);
            this->fetchChatGroupMemberListSuccessCallback = successCallback;
            this->fetchChatGroupMemberListFailureCallback = failureCallback;
            this->sessionModule->sendMessage(fetchChatGroupMemberListRequest);
        }

        void GroupChatModule::fetchCurrentUserChatGroupList(FetchCurrentUserChatGroupListSuccessCallback successCallback,FetchCurrentUserChatGroupListFailureCallback failureCallback){
            Node::FetchChatGroupListRequest fetchChatGroupListRequest;
            this->fetchCurrentUserChatGroupListSuccessCallback = successCallback;
            this->fetchCurrentUserChatGroupListFailureCallback = failureCallback;
	    this->sessionModule->sendMessage(fetchChatGroupListRequest);
        }
        void GroupChatModule::sendGroupChatMessage(const GroupChatMessage & groupChatMessage)
        {
            kakaIM::Node::GroupChatMessage message;
            message.set_groupid(groupChatMessage.getGroupId());
            message.set_sender(groupChatMessage.getSenderId());
            message.set_content(groupChatMessage.getContent());
            this->sessionModule->sendMessage(message);
        }
    }
}
