//
// Created by Kakawater on 2018/3/8
//

#ifndef KAKAIMCLUSTER_GROUPCHATMODULE_H
#define KAKAIMCLUSTER_GROUPCHATMODULE_H

#include "../ServiceModule/ServiceModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "GroupChatModule.h"
#include "GroupChatMessage.h"
#include "ChatGroup.h"
#include "../RosterModule/User.h"
#include "GroupChatMessageDelegate.h"
namespace kakaIM {
    namespace client {
        class GroupChatModule : public ServiceModule {
        public:
            GroupChatModule():sessionModule(nullptr){

            }
            void setCurrentUserAccount(std::string userAccout){
                this->currentUserAccount = userAccout;
            }
            void setDelegate(std::weak_ptr<GroupChatMessageDelegate> delegate){
                this->mGroupChatMessageDelegatePtr = delegate;
            }
            typedef std::function<void(GroupChatModule &,std::string groupId)> CreateChatGroupSuccessCallback;
            enum CreateChatGroupFailureType{
                CreateChatGroupFailureType_NetworkError,//网络异常
                CreateChatGroupFailureType_ServerInteralError//服务器内部错误
            };
            typedef std::function<void(GroupChatModule &,enum CreateChatGroupFailureType failureType)> CreateChatGroupFailureCallback;
            void createChatGroup(const std::string groupName,const std::string groupDescription,CreateChatGroupSuccessCallback successCallback,CreateChatGroupFailureCallback failureCallback);
            typedef std::function<void(GroupChatModule &)> DisbandChatGroupSuccessCallback;
            enum DisbandChatGroupFailureType{
                DisbandChatGroupFailureType_IllegalOperate,//非法操作
            };
            typedef std::function<void(GroupChatModule &,enum DisbandChatGroupFailureType failureType)> DisbandChatGroupFailureCallback;
            void disbandChatGroup(const std::string groupId,DisbandChatGroupSuccessCallback successCallback,DisbandChatGroupFailureCallback failureCallback);
            typedef std::function<void(GroupChatModule &)> JoinChatGroupSuccessCallback;
            enum JoinChatGroupFailureType{
                JoinChatGroupFailureType_NotAllow
            };
            typedef std::function<void(GroupChatModule &,enum JoinChatGroupFailureType failureType)> JoinChatGroupFailureCallback;
            void joinChatGroup(const std::string groupid,JoinChatGroupSuccessCallback successCallback,JoinChatGroupFailureCallback failureCallback);
            typedef std::function<void(GroupChatModule &)> QuitChatGroupSuccessCallback;
            enum QuitChatGroupFailureType{
                QuitChatGroupFailureType_IllegalOperate,//非法操作
            };
            typedef std::function<void(GroupChatModule & ,QuitChatGroupFailureType failureType)> QuitChatGroupFailureCallback;
            void quitChatGroup(const std::string groupId,QuitChatGroupSuccessCallback successCallback,QuitChatGroupFailureCallback failureCallback);
            typedef std::function<void(GroupChatModule &)> UpdateChatGroupInfoSuccessCallback;
            enum UpdateChatGroupInfoFailureType{
                UpdateChatGroupInfoFailureType_IllegalOperate,//非法操作
            };
            typedef std::function<void(GroupChatModule &,enum UpdateChatGroupInfoFailureType failureType)> UpdateChatGroupInfoFailureCallback;
            void updateChatGroupInfo(const std::string groupId,const std::string groupName,const std::string groupDescription,UpdateChatGroupInfoSuccessCallback successCallback,UpdateChatGroupInfoFailureCallback failureCallback);
            typedef std::function<void(GroupChatModule &,client::ChatGroup)> FetchChatGroupInfoSuccessCallback;
            enum FetchChatGroupInfoFailureType{
                FetchChatGroupInfoFailureType_NetworkError,//网络异常
                FetchChatGroupInfoFailureType_ServerInteralError//服务器内部错误
            };
            typedef std::function<void(GroupChatModule &,enum FetchChatGroupInfoFailureType failureType)> FetchChatGroupInfoFailureCallback;
            void fetchChatGroupInfo(std::string groupID,FetchChatGroupInfoSuccessCallback successCallback,FetchChatGroupInfoFailureCallback failureCallback);
            typedef std::function<void(GroupChatModule &,const std::list<std::pair<std::string,std::string>> &)> FetchChatGroupMemberListSuccessCallback;
            enum FetchChatGroupMemberListFailureType{
                FetchChatGroupMemberListFailureType_NetworkError,//网络异常
                FetchChatGroupMemberListFailureType_ServerInteralError,//服务器内部错误
                FetchChatGroupMemberListFailureType_IllegalOperate//非法操作
            };
            typedef std::function<void(GroupChatModule &,enum FetchChatGroupMemberListFailureType failureType)> FetchChatGroupMemberListFailureCallback;
            void fetchChatGroupMemberList(const std::string groupID,FetchChatGroupMemberListSuccessCallback successCallback,FetchChatGroupMemberListFailureCallback failureCallback);
            typedef std::function<void(GroupChatModule &,const std::list<ChatGroup> &)> FetchCurrentUserChatGroupListSuccessCallback;
            enum FetchChatGroupListFailureType{
                FetchChatGroupListFailureType_NetworkError,//网络异常
                FetchChatGroupListFailureType_ServerInteralError,//服务器内部错误
            };
            typedef std::function<void(GroupChatModule &,enum FetchChatGroupListFailureType failureType)> FetchCurrentUserChatGroupListFailureCallback;
            void fetchCurrentUserChatGroupList(FetchCurrentUserChatGroupListSuccessCallback successCallback,FetchCurrentUserChatGroupListFailureCallback failureCallback);
            void sendGroupChatMessage(const GroupChatMessage & groupChatMessage);
        protected:
            virtual void setSessionModule(SessionModule & sessionModule) override;

            virtual std::list<std::string> messageTypes() override;

            virtual void handleMessage(const ::google::protobuf::Message & message,SessionModule & sessionModule) override;
        private:
            void handleChatGroupCreateResponseMessage(const Node::ChatGroupCreateResponse & message);
            void handleChatGroupDisbandResponseMessage(const Node::ChatGroupDisbandResponse & message);
            void handleChatGroupJoinResponseMessage(const Node::ChatGroupJoinResponse & message);
            void handleChatGroupQuitResponseMessage(const Node::ChatGroupQuitResponse & message);
            void handleUpdateChatGroupInfoResponseMessage(const Node::UpdateChatGroupInfoResponse & message);
            void handleFetchChatGroupInfoResponseMessage(const Node::FetchChatGroupInfoResponse & message);
            void handleFetchChatGroupMemberListResponseMessage(const Node::FetchChatGroupMemberListResponse & message);
            void handleFetchChatGroupListResponseMessage(const Node::FetchChatGroupListResponse & message);
            void handleGroupChatMessage(const Node::GroupChatMessage & message);
            SessionModule * sessionModule;
            std::weak_ptr<GroupChatMessageDelegate> mGroupChatMessageDelegatePtr;

            CreateChatGroupSuccessCallback createChatGroupSuccessCallback;
            CreateChatGroupFailureCallback createChatGroupFailureCallback;
            DisbandChatGroupSuccessCallback disbandChatGroupSuccessCallback;
            DisbandChatGroupFailureCallback disbandChatGroupFailureCallback;
            JoinChatGroupSuccessCallback joinChatGroupSuccessCallback;
            JoinChatGroupFailureCallback joinChatGroupFailureCallback;
            QuitChatGroupSuccessCallback quitChatGroupSuccessCallback;
            QuitChatGroupFailureCallback quitChatGroupFailureCallback;
            UpdateChatGroupInfoSuccessCallback updateChatGroupInfoSuccessCallback;
            UpdateChatGroupInfoFailureCallback updateChatGroupInfoFailureCallback;
            FetchChatGroupInfoSuccessCallback fetchChatGroupInfoSuccessCallback;
            FetchChatGroupInfoFailureCallback fetchChatGroupInfoFailureCallback;
            FetchChatGroupMemberListSuccessCallback fetchChatGroupMemberListSuccessCallback;
            FetchChatGroupMemberListFailureCallback fetchChatGroupMemberListFailureCallback;
            FetchCurrentUserChatGroupListSuccessCallback fetchCurrentUserChatGroupListSuccessCallback;
            FetchCurrentUserChatGroupListFailureCallback fetchCurrentUserChatGroupListFailureCallback;

            std::string currentUserAccount;
        };
    }
}
#endif //KAKAIMCLUSTER_GroupChatMODULE_H
