//
// Created by Kakawater on 2018/1/31.
//

#include "KakaIMClient.h"
#include <iostream>
#include "../Common/proto/KakaIMMessage.pb.h"

namespace kakaIM {
    namespace client {
        KakaIMClient::KakaIMClient(const std::string &serverAddr, uint32_t serverPort) :
                sessionModule(nullptr), authenticationModule(nullptr) {
            this->sessionModule = new SessionModule(serverAddr, serverPort);
            this->authenticationModule = new AuthenticationModule();
            this->onlineStateModule = new OnlineStateModule();
            this->rosterModule = new RosterModule();
            this->singleChatModule = new SingleChatModule();
            this->groupChatModule = new GroupChatModule();
            this->sessionModule->addServiceModule(this->authenticationModule);
            this->sessionModule->addServiceModule(this->onlineStateModule);
            this->sessionModule->addServiceModule(this->rosterModule);
            this->sessionModule->addServiceModule(this->singleChatModule);
            this->sessionModule->addServiceModule(this->groupChatModule);
            if (this->sessionModule->init()) {
                this->sessionModule->start();
            } else {
                std::cout << "KakaIMClient::" << __FUNCTION__ << " sessionModule初始化失败" << std::endl;
            }
        }

        KakaIMClient::~KakaIMClient() {
            this->sessionModule->stop();
            if (nullptr != this->authenticationModule) {
                delete this->authenticationModule;
                this->authenticationModule = nullptr;
            }
            if (nullptr != this->onlineStateModule) {
                delete this->onlineStateModule;
                this->onlineStateModule = nullptr;
            }
            if (nullptr != this->rosterModule) {
                delete this->rosterModule;
                this->rosterModule = nullptr;
            }
            if (nullptr != this->singleChatModule) {
                delete this->singleChatModule;
                this->singleChatModule = nullptr;
            }
            if (nullptr != this->groupChatModule) {
                delete this->groupChatModule;
                this->groupChatModule = nullptr;
            }
            if (nullptr != this->sessionModule) {
                delete this->sessionModule;
                this->sessionModule = nullptr;
            }
        }

        KakaIMClient *KakaIMClient::create(const KakaIMConfigs &configs) {
            KakaIMClient *imClient = new KakaIMClient(configs.getServerAddr(), configs.getServerPort());

            return imClient;
        }

        void KakaIMClient::login(const std::string &userAccount, const std::string &userPassword,
                                 LoginSuccessCallback loginSuccessCallback,
                                 LoginFailureCallback loginFailureCallback) {

            this->sessionModule->buildSession([userAccount, userPassword, loginSuccessCallback, loginFailureCallback, this](
                            SessionModule & sessionModule) {
                        this->authenticationModule->login(userAccount, userPassword,
                                                          [userAccount, userPassword, loginSuccessCallback, this](
                                                                  AuthenticationModule &module) {
                                                              this->pullChatMessage();
                                                              this->currentUserAccount = userAccount;
                                                              this->setUserOnlineState(User::UserOnlineState_Online);
                                                              this->rosterModule->setCurrentUserAccount(userAccount);
                                                              if (loginSuccessCallback) {
                                                                  loginSuccessCallback(*this);
                                                              }
                                                          }, [userAccount, userPassword, loginFailureCallback, this](
                                        AuthenticationModule &module,
                                        AuthenticationModule::LoginFailureReasonType failureReasonType) {
                                    std::cout<<"登陆失败"<<std::endl;
                                    if (loginFailureCallback) {
                                        switch (failureReasonType) {
                                            case AuthenticationModule::LoginFailureReasonType_ServerInternalError: {
                                                loginFailureCallback(*this,LoginFailureReasonType_ServerInternalError);
                                            }
                                                break;
                                            case AuthenticationModule::LoginFailureReasonType_WrongAccountOrPassword: {
                                                loginFailureCallback(*this,LoginFailureReasonType_WrongAccountOrPassword);
                                            }
                                                break;
                                        }
                                    }
                                });
                    }, [userAccount, userPassword, loginFailureCallback,this](SessionModule &sessionModule,
                                                                         SessionModule::BuildSessionFailureReasonType failureReasonType) {
                        //构建会话失败
                        if (loginFailureCallback) {
                            loginFailureCallback(*this,LoginFailureReasonType_NetworkError);
                        }
                    });
        }

        void KakaIMClient::registerAccount(const std::string &userAccount, const std::string &userPassword,
                                           RegisterAccountSuccessCallback successCallback,
                                           RegisterAccountFailureCallback failureCallback) {
            this->sessionModule->buildSession(
                    [this, userAccount, userPassword, successCallback, failureCallback](SessionModule & sessionModule) {
                        //构建会话成功
                        this->authenticationModule->registerAccount(userAccount, userPassword,
                                                                    [this, userAccount, userPassword, successCallback](
                                                                            AuthenticationModule & module) {

                                                                        if (successCallback) {
                                                                            successCallback(*this);
                                                                        }
                                                                    },
                                                                    [this, userAccount, userPassword, failureCallback](
                                                                            AuthenticationModule & module,
                                                                            AuthenticationModule::RegisterFailureReasonType failureReasonType) {
                                                                        if (failureCallback) {
                                                                            switch (failureReasonType) {
                                                                                case AuthenticationModule::RegisterFailureReasonType_UserAccountDuplicate: {
                                                                                    failureCallback(*this,RegisterAccountFailureReasonType_UserAccountDuplicate);
                                                                                }
                                                                                    break;
                                                                                case AuthenticationModule::RegisterFailureReasonType_ServerInteralError: {
                                                                                    failureCallback(*this,RegisterAccountFailureReasonType_ServerInteralError);
                                                                                }
                                                                                    break;
                                                                                case AuthenticationModule::RegisterFailureReasonType_NetworkError: {
                                                                                    failureCallback(*this,RegisterAccountFailureReasonType_NetworkError);
                                                                                }
                                                                                    break;
                                                                            }
                                                                        }
                                                                    });


                    }, [userAccount, userPassword, failureCallback, this](SessionModule &sessionModule,
                                                                          SessionModule::BuildSessionFailureReasonType failureReasonType) {
                        //构建会话失败
                        if (failureCallback) {
                            failureCallback(*this,RegisterAccountFailureReasonType_NetworkError);
                        }
                    });
        }

        void KakaIMClient::logout() {

        }

        void KakaIMClient::setUserOnlineState(User::UserOnlineState onlineState) {
            Node::OnlineStateMessage onlineStateMessage;
            onlineStateMessage.set_useraccount(this->currentUserAccount);
            switch (onlineState) {
                case User::UserOnlineState_Online: {
                    onlineStateMessage.set_userstate(kakaIM::Node::OnlineStateMessage_OnlineState_Online);
                    std::cout << "KakaIMClient::" << __FUNCTION__ << "设置为在线" << std::endl;
                }
                    break;
                case User::UserOnlineState_Invisible: {
                    onlineStateMessage.set_userstate(kakaIM::Node::OnlineStateMessage_OnlineState_Invisible);
                    std::cout << "KakaIMClient::" << __FUNCTION__ << "设置为隐身" << std::endl;
                }
                    break;
                default: {//默认不作任何处理，包括离线操作
                    return;
                }
            }
            this->sessionModule->sendMessage(*((const ::google::protobuf::Message *) &onlineStateMessage));
            this->currentUserOnlineState = onlineState;
        }

        User::UserOnlineState KakaIMClient::getUserOnlineState() {
            return this->currentUserOnlineState;
        }

        void KakaIMClient::fetchCurrentUserVCard(FetchCurrentUserVCardSuccessCallback successCallback,
                                                 FetchCurrentUserVCardFailureCallback failureCallback) {
            User user(this->currentUserAccount);
            this->rosterModule->fetchUserVCard(user, [successCallback, this](RosterModule &rosterModule,
                                                                             const UserVCard &userVcard) {
                if (successCallback) {
                    successCallback(*this, userVcard);
                }
            }, [failureCallback, this](RosterModule &rosterModule,
                                       RosterModule::FetchUserVCardFailureType failureTyoe) {
                if (failureCallback) {
                    switch (failureTyoe) {
                        case RosterModule::FetchUserVCardFailureType_NetworkError: {

                            failureCallback(*this, FetchCurrentUserVCardFailureType_NetworkError);
                        }
                            break;
                        case RosterModule::FetchUserVCardFailureType_ServerInteralError: {

                            failureCallback(*this, FetchCurrentUserVCardFailureType_ServerInteralError);
                        }
                            break;
                    }
                }
            });
        }

        void KakaIMClient::updateCurrentUserVCard(UserVCard &userVCard, UpdateUserVCardSuccessCallback successCallback, UpdateUserVCardFailureCallback failureCallback) {
            this->rosterModule->updateCurrentUserVCard(userVCard, [successCallback, this](RosterModule &rosterModule) {
                if (successCallback) {
                    successCallback(*this);
                }
            }, [failureCallback, this](RosterModule &rosterModule,
                                       RosterModule::UpdateCurrentUserVCardFailureType failureType) {
                if (failureCallback) {
                    switch (failureType) {
                        case RosterModule::UpdateUserVCardFailureType_UserVCardIllegal: {
                            failureCallback(*this, UpdateCurrentUserVCardFailureType_UserVCardIllegal);
                        }
                            break;
                        case RosterModule::UpdateUserVCardFailureType_NetworkError: {
                            failureCallback(*this,UpdateCurrentUserVCardFailureType_NetworkError);
                        }
                            break;
                        case RosterModule::UpdateUserVCardFailureType_ServerInteralError: {
                            failureCallback(*this,UpdateCurrentUserVCardFailureType_ServerInteralError);
                        }
                            break;
                    }
                }
            });
        }

        void KakaIMClient::fetchFriendList(FetchFriendListSuccessCallback successCallback, FetchFriendListFailureCallback failureCallback) {
            this->rosterModule->fetchFriendList(
                    [successCallback, this](RosterModule &rosterModule, std::list<User> friendList) {
                        if (successCallback) {
                            successCallback(*this, friendList);
                        }
                    }, [failureCallback, this](RosterModule &rosterModule,
                                               RosterModule::FetchFriendListFailureType failureType) {
                        if (failureCallback) {
                            switch (failureType) {
                                case RosterModule::FetchFriendListFailureType_NetworkError: {
                                    failureCallback(*this,FetchFriendListFailureType_NetworkError);
                                }
                                    break;
                                case RosterModule::FetchFriendListFailureType_ServerInteralError: {
                                    failureCallback(*this,FetchFriendListFailureType_ServerInteralError);
                                }
                                    break;
                            }
                        }
                    });
        }

        void KakaIMClient::setFriendRelationDelegate(std::weak_ptr<FriendRelationDelegate> friendRelationDelegate) {
            this->rosterModule->setFriendRelationDelegate(friendRelationDelegate);
        }

        void KakaIMClient::sendFriendRequest(const FriendRequest request) {
            this->rosterModule->sendFriendRequest(request);
        }

        void KakaIMClient::andswerToFriendRequest(const FriendRequestAnswer answer) {
            this->rosterModule->andswerToFriendRequest(answer);
        }

        void KakaIMClient::deleteFriend(const User user,DeleteFriendSuccessCallback successCallback, DeleteFriendFailureCallback failureCallback) {
            this->rosterModule->deleteFriend(user, [successCallback, this](RosterModule &rosterModule) {
                if (successCallback) {
                    successCallback(*this);
                }
            }, [failureCallback, this](RosterModule &rosterModule,
                                       RosterModule::DeleteFriendFailureType failureType) {
                if (failureCallback) {
                    switch (failureType) {
                        case RosterModule::DeleteFriendFailureType_NetworkError: {
                            failureCallback(*this,DeleteFriendFailureType_NetworkError);
                        }
                            break;
                        case RosterModule::DeleteFriendFailureType_ServerInteralError: {
                            failureCallback(*this,DeleteFriendFailureType_ServerInteralError);
                        }
                            break;
                        case RosterModule::DeleteFriendFailureType_FriendRelationNotExitBefore: {
                            failureCallback(*this,DeleteFriendFailureType_FriendRelationNotExitBefore);
                        }
                            break;
                    }
                }
            });
        }

        void
        KakaIMClient::setSingleChatMessageDelegate(std::weak_ptr<SingleChatMessageDelegate> singleChatMessageDelegate) {
            this->singleChatModule->setDelegate(singleChatMessageDelegate);
        }

        void KakaIMClient::sendChatMessage(const std::string content, const std::string targetUserAccount) {
            this->singleChatModule->sendMessage(ChatMessage(this->currentUserAccount, targetUserAccount, content));
        }

        void KakaIMClient::pullChatMessage() {
            Node::PullChatMessage message;
            message.set_messageid(0);
            this->sessionModule->sendMessage(message);
        }

        void KakaIMClient::setGroupChatMessageDelegate(std::weak_ptr<GroupChatMessageDelegate> delegate) {
            this->groupChatModule->setDelegate(delegate);
        }

        void KakaIMClient::createChatGroup(const std::string groupName, const std::string groupDescription,
                                           CreateChatGroupSuccessCallback successCallback,
                                           CreateChatGroupFailureCallback failureCallback) {
            this->groupChatModule->createChatGroup(groupName, groupDescription,
                                                   [this, successCallback](GroupChatModule &groupChatModule,
                                                                           std::string groupId) {
                                                       if (successCallback) {
                                                           successCallback(*this, groupId);
                                                       }
                                                   }, [this, failureCallback](GroupChatModule &groupChatModule,
                                                                              enum GroupChatModule::CreateChatGroupFailureType failureType) {
                        if (failureCallback) {
                            switch (failureType) {
                                case GroupChatModule::CreateChatGroupFailureType_NetworkError: {
                                    failureCallback(*this, CreateChatGroupFailureType_NetworkError);
                                }
                                    break;
                                case GroupChatModule::CreateChatGroupFailureType_ServerInteralError: {
                                    failureCallback(*this, CreateChatGroupFailureType_ServerInteralError);
                                }
                                    break;
                                default: {

                                }
                            }
                        }
                    });
        }

        void KakaIMClient::disbandChatGroup(const std::string groupId, DisbandChatGroupSuccessCallback successCallback,
                                            DisbandChatGroupFailureCallback failureCallback) {
            this->groupChatModule->disbandChatGroup(groupId, [this, successCallback](GroupChatModule &groupChatModule) {
                if (successCallback) {
                    successCallback(*this);
                }
            }, [this, failureCallback](GroupChatModule &,
                                       enum GroupChatModule::DisbandChatGroupFailureType failureType) {
                if (failureCallback) {
                    switch (failureType) {
                        case GroupChatModule::DisbandChatGroupFailureType_IllegalOperate: {
                            failureCallback(*this, DisbandChatGroupFailureType_IllegalOperate);
                        }
                            break;
                        default: {

                        }
                    }
                }
            });
        }

        void KakaIMClient::joinChatGroup(const std::string groupid, JoinChatGroupSuccessCallback successCallback,
                                         JoinChatGroupFailureCallback failureCallback) {
            this->groupChatModule->joinChatGroup(groupid, [this, successCallback](GroupChatModule &groupChatModule) {
                if (successCallback) {
                    successCallback(*this);
                }
            }, [this, failureCallback](GroupChatModule &groupChatModule,
                                       enum GroupChatModule::JoinChatGroupFailureType failureType) {
                if (failureCallback) {
                    switch (failureType) {
                        case GroupChatModule::JoinChatGroupFailureType_NotAllow: {
                            failureCallback(*this, JoinChatGroupFailureType_NotAllow);
                        }
                            break;
                        default: {

                        }
                    }
                }
            });
        }

        void KakaIMClient::quitChatGroup(const std::string groupId, QuitChatGroupSuccessCallback successCallback,
                                         QuitChatGroupFailureCallback failureCallback) {
            this->groupChatModule->quitChatGroup(groupId, [this, successCallback](GroupChatModule &groupChatModule) {
                if (successCallback) {
                    successCallback(*this);
                }
            }, [this, failureCallback](GroupChatModule &groupChatModule,
                                       GroupChatModule::QuitChatGroupFailureType failureType) {
                if (failureCallback) {
                    switch (failureType) {
                        case GroupChatModule::QuitChatGroupFailureType_IllegalOperate: {
                            failureCallback(*this, QuitChatGroupFailureType_IllegalOperate);
                        }
                            break;
                        default: {

                        }
                    }
                }
            });
        }

        void KakaIMClient::updateChatGroupInfo(ChatGroup group, UpdateChatGroupInfoSuccessCallback successCallback,
                                               UpdateChatGroupInfoFailureCallback failureCallback) {
            this->groupChatModule->updateChatGroupInfo(group.getID(), group.getName(), group.getDescription(),
                                                       [this, successCallback](GroupChatModule &groupChatModule) {
                                                           if (successCallback) {
                                                               successCallback(*this);
                                                           }
                                                       }, [this, failureCallback](GroupChatModule &groupChatModule,
                                                                                  enum GroupChatModule::UpdateChatGroupInfoFailureType failureType) {
                        if (failureCallback) {
                            switch (failureType) {
                                case GroupChatModule::UpdateChatGroupInfoFailureType_IllegalOperate: {
                                    failureCallback(*this, UpdateChatGroupInfoFailureType_IllegalOperate);
                                }
                                    break;
                                default: {

                                }
                            }
                        }
                    });
        }

        void KakaIMClient::fetchChatGroupInfo(std::string groupID, FetchChatGroupInfoSuccessCallback successCallback,
                                              FetchChatGroupInfoFailureCallback failureCallback) {
            this->groupChatModule->fetchChatGroupInfo(groupID, [this, successCallback](GroupChatModule &groupChatModule,
                                                                                       client::ChatGroup group) {
                if (successCallback) {
                    successCallback(*this, group);
                }
            }, [this, failureCallback](GroupChatModule &groupChatModule,
                                       enum GroupChatModule::FetchChatGroupInfoFailureType failureType) {
                if (failureCallback) {
                    switch (failureType) {
                        case GroupChatModule::FetchChatGroupInfoFailureType_NetworkError: {
                            failureCallback(*this, FetchChatGroupInfoFailureType_NetworkError);
                        }
                            break;
                        case GroupChatModule::FetchChatGroupInfoFailureType_ServerInteralError: {
                            failureCallback(*this, FetchChatGroupInfoFailureType_ServerInteralError);
                        }
                            break;
                        default: {

                        }
                    }
                }
            });
        }

        void KakaIMClient::fetchChatGroupMemberList(const std::string groupID,
                                                    FetchChatGroupMemberListSuccessCallback successCallback,
                                                    FetchChatGroupMemberListFailureCallback failureCallback) {
            this->groupChatModule->fetchChatGroupMemberList(groupID,
                                                            [this, successCallback](GroupChatModule &groupChatModule,
                                                                                    const std::list<std::pair<std::string, std::string>> &memberList) {
                                                                if (successCallback) {
                                                                    successCallback(*this, memberList);
                                                                }
                                                            }, [this, failureCallback](GroupChatModule &,
                                                                                       enum GroupChatModule::FetchChatGroupMemberListFailureType failureType) {
                        if (failureCallback) {
                            switch (failureType) {
                                case GroupChatModule::FetchChatGroupMemberListFailureType_NetworkError: {
                                    failureCallback(*this, FetchChatGroupMemberListFailureType_NetworkError);
                                }
                                    break;

                                case GroupChatModule::FetchChatGroupMemberListFailureType_ServerInteralError: {
                                    failureCallback(*this, FetchChatGroupMemberListFailureType_ServerInteralError);
                                }
                                    break;
                                case GroupChatModule::FetchChatGroupMemberListFailureType_IllegalOperate: {
                                    failureCallback(*this, FetchChatGroupMemberListFailureType_IllegalOperate);
                                }
                                    break;
                                default: {

                                }
                            }
                        }
                    });
        }

        void KakaIMClient::fetchCurrentUserChatGroupList(FetchCurrentUserChatGroupListSuccessCallback successCallback,
                                                         FetchCurrentUserChatGroupListFailureCallback failureCallback) {
            this->groupChatModule->fetchCurrentUserChatGroupList(
                    [this, successCallback](GroupChatModule &groupChatModule, const std::list<ChatGroup> &groupList) {
                        if (successCallback) {
                            successCallback(*this, groupList);
                        }
                    }, [this, failureCallback](GroupChatModule &groupChatModule,
                                               enum GroupChatModule::FetchChatGroupListFailureType failureType) {
                        if (failureCallback) {
                            switch (failureType) {
                                case GroupChatModule::FetchChatGroupListFailureType_NetworkError: {
                                    failureCallback(*this, FetchChatGroupListFailureType_NetworkError);
                                }
                                    break;
                                case GroupChatModule::FetchChatGroupListFailureType_ServerInteralError: {
                                    failureCallback(*this, FetchChatGroupListFailureType_ServerInteralError);
                                }
                                    break;
                                default: {

                                }
                            }
                        }
                    });
        }

        void KakaIMClient::sendGroupChatMessage(const GroupChatMessage &groupChatMessage) {
            this->groupChatModule->sendGroupChatMessage(groupChatMessage);
        }
    }
}

