//
// Created by Kakawater on 2018/1/31.
//

#ifndef KAKAIMCLUSTER_KAKAIMCLIENT_H
#define KAKAIMCLUSTER_KAKAIMCLIENT_H

#include <string>
#include <Client/KakaIMConfigs.h>
#include <Client/SessionModule/SessionModule.h>
#include <Client/AuthenticationModule/AuthenticationModule.h>
#include <Client/OnlineStateModule/OnlineStateModule.h>
#include <Client/ServiceModule/ServiceModule.h>
#include <Client/RosterModule/RosterModule.h>
#include <Client/SingleChatModule/SingleChatModule.h>
#include <Client/GroupChatModule/GroupChatModule.h>

namespace kakaIM {
    namespace client {
        class KakaIMClient {
        public:
            typedef std::function<void(KakaIMClient &imClient,
                                       const UserVCard &userVcard)> FetchCurrentUserVCardSuccessCallback;
            enum FetchCurrentUserVCardFailureType {
                FetchCurrentUserVCardFailureType_ServerInteralError,//服务器内部错误
                FetchCurrentUserVCardFailureType_NetworkError//网络连接错误
            };
            typedef std::function<void(KakaIMClient &imClient,
                                       FetchCurrentUserVCardFailureType failureTyoe)> FetchCurrentUserVCardFailureCallback;
            typedef std::function<void(KakaIMClient &imClient)> UpdateUserVCardSuccessCallback;
            enum UpdateCurrentUserVCardFailureType {
                UpdateCurrentUserVCardFailureType_UserVCardIllegal,//电子名片有问题
                UpdateCurrentUserVCardFailureType_ServerInteralError,//服务器内部错误
                UpdateCurrentUserVCardFailureType_NetworkError//网络连接错误
            };
            typedef std::function<void(KakaIMClient &imClient,
                                       UpdateCurrentUserVCardFailureType failureType)> UpdateUserVCardFailureCallback;
            typedef std::function<void(KakaIMClient &rosterModule,
                                       std::list<User> friendList)> FetchFriendListSuccessCallback;
            enum FetchFriendListFailureType {
                FetchFriendListFailureType_ServerInteralError,//服务器内部错误
                FetchFriendListFailureType_NetworkError//网络连接错误
            };
            typedef std::function<void(KakaIMClient &rosterModule,
                                       FetchFriendListFailureType failureType)> FetchFriendListFailureCallback;

            KakaIMClient(const KakaIMClient &) = delete;

            KakaIMClient &operator=(const KakaIMClient &) = delete;

            virtual ~KakaIMClient();

            static KakaIMClient *create(const KakaIMConfigs &configs);

#pragma - 登陆

            typedef std::function<void(KakaIMClient &imClient)> LoginSuccessCallback;
            enum LoginFailureReasonType {
                LoginFailureReasonType_NetworkError,//网络连接错误
                LoginFailureReasonType_WrongAccountOrPassword,//账号或者密码错误
                LoginFailureReasonType_ServerInternalError//服务器内部错误
            };
            typedef std::function<void(KakaIMClient &imClient,
                                       LoginFailureReasonType failureReasonType)> LoginFailureCallback;

            void login(const std::string &userName, const std::string &userPassword,
                       LoginSuccessCallback loginSuccessCallback, LoginFailureCallback loginFailureCallback);

#pragma - 注册

            typedef std::function<void(KakaIMClient &imClient)> RegisterAccountSuccessCallback;
            enum RegisterAccountFailureReasonType {
                RegisterAccountFailureReasonType_UserAccountDuplicate,//用户名已存在
                RegisterAccountFailureReasonType_ServerInteralError,//服务器内部错误
                RegisterAccountFailureReasonType_NetworkError//网络连接错误
            };
            typedef std::function<void(KakaIMClient &imClient,
                                       RegisterAccountFailureReasonType failureReasonType)> RegisterAccountFailureCallback;

            void registerAccount(const std::string &userName, const std::string &userPassword,
                                 RegisterAccountSuccessCallback successCallback,
                                 RegisterAccountFailureCallback failureCallback);

#pragma - 离线

            void logout();

#pragma - 用户状态
            void setUserOnlineState(User::UserOnlineState onlineState);

            User::UserOnlineState getUserOnlineState();

#pragma - 花名册

            void fetchCurrentUserVCard(FetchCurrentUserVCardSuccessCallback successCallback,
                                       FetchCurrentUserVCardFailureCallback failureCallback);

            void
            updateCurrentUserVCard(UserVCard &userVCard, UpdateUserVCardSuccessCallback successCallback, UpdateUserVCardFailureCallback failureCallback);

            void fetchFriendList(FetchFriendListSuccessCallback successCallback, FetchFriendListFailureCallback failureCallback);

            void setFriendRelationDelegate(std::weak_ptr<FriendRelationDelegate> friendRelationDelegate);

            void sendFriendRequest(const FriendRequest request);

            void andswerToFriendRequest(const FriendRequestAnswer answer);

            typedef std::function<void(KakaIMClient &imClient)> DeleteFriendSuccessCallback;
            enum DeleteFriendFailureType {
                DeleteFriendFailureType_ServerInteralError,//服务器内部错误
                DeleteFriendFailureType_NetworkError,//网络连接错误
                DeleteFriendFailureType_FriendRelationNotExitBefore,//好友关系事先不存在
            };
            typedef std::function<void(KakaIMClient &imClient,
                                       DeleteFriendFailureType failureType)> DeleteFriendFailureCallback;

            void deleteFriend(const User user, DeleteFriendSuccessCallback successCallback,
                              DeleteFriendFailureCallback failureCallback);

#pragma - 单聊

            void sendChatMessage(const std::string content, const std::string targetUserAccount);

            void setSingleChatMessageDelegate(std::weak_ptr<SingleChatMessageDelegate> singleChatMessageDelegate);

            void pullChatMessage();

            void setGroupChatMessageDelegate(std::weak_ptr<GroupChatMessageDelegate> delegate);

            typedef std::function<void(KakaIMClient &, std::string groupId)> CreateChatGroupSuccessCallback;
            enum CreateChatGroupFailureType {
                CreateChatGroupFailureType_NetworkError,//网络异常
                CreateChatGroupFailureType_ServerInteralError//服务器内部错误
            };
            typedef std::function<void(KakaIMClient &,
                                       enum CreateChatGroupFailureType failureType)> CreateChatGroupFailureCallback;

            void createChatGroup(const std::string groupName, const std::string groupDescription,
                                 CreateChatGroupSuccessCallback successCallback,
                                 CreateChatGroupFailureCallback failureCallback);

            typedef std::function<void(KakaIMClient &)> DisbandChatGroupSuccessCallback;
            enum DisbandChatGroupFailureType {
                DisbandChatGroupFailureType_IllegalOperate,//非法操作
            };
            typedef std::function<void(KakaIMClient &,
                                       enum DisbandChatGroupFailureType failureType)> DisbandChatGroupFailureCallback;

            void disbandChatGroup(const std::string groupId, DisbandChatGroupSuccessCallback successCallback,
                                  DisbandChatGroupFailureCallback failureCallback);

            typedef std::function<void(KakaIMClient &)> JoinChatGroupSuccessCallback;
            enum JoinChatGroupFailureType {
                JoinChatGroupFailureType_NotAllow
            };
            typedef std::function<void(KakaIMClient &,
                                       enum JoinChatGroupFailureType failureType)> JoinChatGroupFailureCallback;

            void joinChatGroup(const std::string groupid, JoinChatGroupSuccessCallback successCallback,
                               JoinChatGroupFailureCallback failureCallback);

            typedef std::function<void(KakaIMClient &)> QuitChatGroupSuccessCallback;
            enum QuitChatGroupFailureType {
                QuitChatGroupFailureType_IllegalOperate,//非法操作
            };
            typedef std::function<void(KakaIMClient &,
                                       QuitChatGroupFailureType failureType)> QuitChatGroupFailureCallback;

            void quitChatGroup(const std::string groupId, QuitChatGroupSuccessCallback successCallback,
                               QuitChatGroupFailureCallback failureCallback);

            typedef std::function<void(KakaIMClient &)> UpdateChatGroupInfoSuccessCallback;
            enum UpdateChatGroupInfoFailureType {
                UpdateChatGroupInfoFailureType_IllegalOperate,//非法操作
            };
            typedef std::function<void(KakaIMClient &,
                                       enum UpdateChatGroupInfoFailureType failureType)> UpdateChatGroupInfoFailureCallback;

            void updateChatGroupInfo(ChatGroup group, UpdateChatGroupInfoSuccessCallback successCallback,
                                     UpdateChatGroupInfoFailureCallback failureCallback);

            typedef std::function<void(KakaIMClient &, client::ChatGroup)> FetchChatGroupInfoSuccessCallback;
            enum FetchChatGroupInfoFailureType {
                FetchChatGroupInfoFailureType_NetworkError,//网络异常
                FetchChatGroupInfoFailureType_ServerInteralError//服务器内部错误
            };
            typedef std::function<void(KakaIMClient &,
                                       enum FetchChatGroupInfoFailureType failureType)> FetchChatGroupInfoFailureCallback;

            void fetchChatGroupInfo(std::string groupID, FetchChatGroupInfoSuccessCallback successCallback,
                                    FetchChatGroupInfoFailureCallback failureCallback);

            typedef std::function<void(KakaIMClient &,
                                       const std::list<std::pair<std::string, std::string>> &)> FetchChatGroupMemberListSuccessCallback;
            enum FetchChatGroupMemberListFailureType {
                FetchChatGroupMemberListFailureType_NetworkError,//网络异常
                FetchChatGroupMemberListFailureType_ServerInteralError,//服务器内部错误
                FetchChatGroupMemberListFailureType_IllegalOperate//非法操作
            };
            typedef std::function<void(KakaIMClient &,
                                       enum FetchChatGroupMemberListFailureType failureType)> FetchChatGroupMemberListFailureCallback;

            void
            fetchChatGroupMemberList(const std::string groupID, FetchChatGroupMemberListSuccessCallback successCallback,
                                     FetchChatGroupMemberListFailureCallback failureCallback);

            typedef std::function<void(KakaIMClient &,
                                       const std::list<ChatGroup> &)> FetchCurrentUserChatGroupListSuccessCallback;
            enum FetchChatGroupListFailureType {
                FetchChatGroupListFailureType_NetworkError,//网络异常
                FetchChatGroupListFailureType_ServerInteralError,//服务器内部错误
            };
            typedef std::function<void(KakaIMClient &,
                                       enum FetchChatGroupListFailureType failureType)> FetchCurrentUserChatGroupListFailureCallback;

            void fetchCurrentUserChatGroupList(FetchCurrentUserChatGroupListSuccessCallback successCallback,
                                               FetchCurrentUserChatGroupListFailureCallback failureCallback);

            void sendGroupChatMessage(const GroupChatMessage &groupChatMessage);

        private:
            KakaIMClient(const std::string &serverAddr, uint32_t serverPort);

            SessionModule *sessionModule;
            AuthenticationModule *authenticationModule;
            OnlineStateModule *onlineStateModule;
            RosterModule *rosterModule;
            SingleChatModule *singleChatModule;
            GroupChatModule *groupChatModule;
            std::string currentUserAccount;
            User::UserOnlineState currentUserOnlineState;
        };
    }
}
#endif //KAKAIMCLUSTER_KAKAIMCLIENT_H

