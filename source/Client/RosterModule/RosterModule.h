//
// Created by Kakawater on 2018/2/1.
//

#ifndef KAKAIMCLUSTER_ROSTERMODULE_H
#define KAKAIMCLUSTER_ROSTERMODULE_H

#include <Client/ServiceModule/ServiceModule.h>
#include <Client/RosterModule/User.h>
#include <Client/RosterModule/RosterDB.h>
#include <Client/RosterModule/FriendRequest.h>
#include <Client/RosterModule/FriendRelationDelegate.h>
#include <Common/proto/KakaIMMessage.pb.h>
#include <memory>
#include <list>

namespace kakaIM {
    namespace client {

        class RosterModule : public ServiceModule {
        public:
            RosterModule():sessionModule(nullptr){

            }
            typedef std::function<void(RosterModule &rosterModule,
                                       const UserVCard &userVcard)> FetchUserVCardSuccessCallback;
            enum FetchUserVCardFailureType {
                FetchUserVCardFailureType_ServerInteralError,//服务器内部错误
                FetchUserVCardFailureType_NetworkError//网络连接错误
            };
            typedef std::function<void(RosterModule &rosterModule,
                                       FetchUserVCardFailureType failureTyoe)> FetchUserVCardFailureCallback;
            typedef std::function<void(RosterModule &rosterModule)> UpdateCurrentUserVCardSuccessCallback;
            enum UpdateCurrentUserVCardFailureType {
                UpdateUserVCardFailureType_UserVCardIllegal,//电子名片有问题
                UpdateUserVCardFailureType_ServerInteralError,//服务器内部错误
                UpdateUserVCardFailureType_NetworkError//网络连接错误
            };
            typedef std::function<void(RosterModule &rosterModule,
                                       UpdateCurrentUserVCardFailureType failureType)> UpdateCurrentUserVCardFailureCallback;

            void fetchUserVCard(User &user, FetchUserVCardSuccessCallback successCallback,
                                FetchUserVCardFailureCallback failureCallback);

            void updateCurrentUserVCard(const UserVCard &userVCard,
                                        UpdateCurrentUserVCardSuccessCallback successCallback,
                                        UpdateCurrentUserVCardFailureCallback failureCallback);

            typedef std::function<void(RosterModule &rosterModule,
                                       std::list<User> friendList)> FetchFriendListSuccessCallback;
            enum FetchFriendListFailureType {
                FetchFriendListFailureType_ServerInteralError,//服务器内部错误
                FetchFriendListFailureType_NetworkError//网络连接错误
            };
            typedef std::function<void(RosterModule &rosterModule,
                                       FetchFriendListFailureType failureType)> FetchFriendListFailureCallback;

            void fetchFriendList(FetchFriendListSuccessCallback successCallback,
                                 FetchFriendListFailureCallback failureCallback);

            void setRosterDB(std::weak_ptr<RosterDB> rosterDB) {
                this->mRosterDBPtr = rosterDB;
            }

            void setFriendRelationDelegate(std::weak_ptr<FriendRelationDelegate> friendRelationDelegate) {
                this->mFriendRelationDelegatePtr = friendRelationDelegate;
            }

            void setCurrentUserAccount(std::string userAccout) {
                this->currentUserAccount = userAccout;
            }

            void sendFriendRequest(const FriendRequest request);

            void andswerToFriendRequest(const FriendRequestAnswer answer);

            typedef std::function<void(RosterModule &rosterModule)> DeleteFriendSuccessCallback;
            enum DeleteFriendFailureType {
                DeleteFriendFailureType_ServerInteralError,//服务器内部错误
                DeleteFriendFailureType_NetworkError,//网络连接错误
                DeleteFriendFailureType_FriendRelationNotExitBefore,//好友关系事先不存在
            };
            typedef std::function<void(RosterModule &rosterModule,
                                       DeleteFriendFailureType failureType)> DeleteFriendFailureCallback;

            void deleteFriend(const User user, DeleteFriendSuccessCallback successCallback,
                              DeleteFriendFailureCallback failureCallback);

        protected:
            virtual void setSessionModule(SessionModule &sessionModule) override;

            virtual std::list<std::string> messageTypes() override;

            virtual void
            handleMessage(const ::google::protobuf::Message &message, SessionModule &sessionModule) override;

        private:
            SessionModule *sessionModule;

            class FetchUserVCardCallbackCompare {
            public:
                bool operator()(
                        const std::pair<std::string, std::pair<FetchUserVCardSuccessCallback, FetchUserVCardFailureCallback>> &left,
                        const std::pair<std::string, std::pair<FetchUserVCardSuccessCallback, FetchUserVCardFailureCallback>> &right) {
                    return left.first < right.first;
                }
            };

            std::set<std::pair<std::string, std::pair<FetchUserVCardSuccessCallback, FetchUserVCardFailureCallback>>, FetchUserVCardCallbackCompare> fetchUserVCardCallbackSet;
            UpdateCurrentUserVCardSuccessCallback updateCurrentUserVCardSuccessCallback;
            UpdateCurrentUserVCardFailureCallback updateCurrentUserVCardFailureCallback;
            FetchFriendListSuccessCallback fetchFriendListSuccessCallback;
            FetchFriendListFailureCallback fetchFriendListFailureCallback;
            DeleteFriendSuccessCallback deleteFriendSuccessCallback;
            DeleteFriendFailureCallback deleteFriendFailureCallback;
            std::string currentUserAccount;
            std::weak_ptr<RosterDB> mRosterDBPtr;
            std::weak_ptr<FriendRelationDelegate> mFriendRelationDelegatePtr;

            void handleUserVCardResponseMessage(const Node::UserVCardResponseMessage &userVCardResponseMessage);

            void handleUpdateUserVCardMessageResponse(
                    const Node::UpdateUserVCardMessageResponse &updateUserVCardMessageResponse);

            void handleFriendListResponseMessage(const Node::FriendListResponseMessage &friendListResponseMessage);

            void handleBuildingRelationshipRequestMessage(
                    const Node::BuildingRelationshipRequestMessage &buildingRelationshipRequestMessage);

            void handleBuildingRelationshipAnswerMessage(
                    const Node::BuildingRelationshipAnswerMessage &buildingRelationshipAnswerMessage);

            void handleDestoryingRelationshipResponseMessage(
                    const Node::DestoryingRelationshipResponseMessage &destoryingRelationshipResponseMessage);
        };
    }
}

#endif //KAKAIMCLUSTER_ROSTERMODULE_H
