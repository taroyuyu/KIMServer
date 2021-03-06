//
// Created by taroyuyu on 2018/1/7.
//

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include "Node.h"
#include "../Common/proto/KakaIMMessage.pb.h"
#include "ClusterModule/ClusterModule.h"
#include "../Common/KakaIMMessageAdapter.h"
#include "SessionModule/SessionModule.h"
#include "AuthenticationModule/AuthenticationModule.h"
#include "OnlineStateModule/OnlineStateModule.h"
#include "MessageSendServiceModule/MessageSendServiceModule.h"
#include "RosterModule/RosterModule.h"
#include "OfflineModule/OfflineModule.h"
#include "SingleChatModule/SingleChatModule.h"
#include "GroupCharModule/GroupChatModule.h"
#include "../Common/yaml-cpp/include/yaml.h"
#include "Log/log.h"
namespace kakaIM {
    namespace node {
        Node::Node() :
                m_task_semaphore(nullptr) {
        }

        bool Node::init(int argc, char *argv[]) {
            std::string listen_address;
            uint16_t listen_port = 1220;
            std::string database_address;
            uint16_t database_port = 0;
            std::string database_dbname;
            std::string database_user;
            std::string database_password;
            std::string node_server_id;
            std::string node_invitation_code;
            std::pair<float, float> lngLatPair;
            std::string president_address;
            uint16_t president_port = 1221;

            bool hasConfigFilePath = false;
            std::string configFilePath;
            int opt = 0;
            while (-1 != (opt = getopt(argc, argv, "f:"))) {
                switch (opt) {
                    case 'f': {
                        configFilePath = optarg;
                        hasConfigFilePath = true;
                    }
                        break;
                }
            }

            if (!hasConfigFilePath) {
                std::cerr << "Usage: " << argv[0] << " -f <configFilePath>" << std::endl;
                return false;
            }

            YAML::Node config = YAML::LoadFile(configFilePath);

            if (!config["network"].IsDefined()) {
                std::cerr << "There is no network setup in the configuration file." << std::endl;
                return false;
            }

            if (!config["network"].IsMap()) {
                std::cerr << "The format of network was incorrect." << std::endl;
                return false;
            } else {
                if (!config["network"]["listen_address"].IsDefined()) {
                    std::cerr << "The listen_address was not specific in the configuration." << std::endl;
                    return false;
                } else {
                    try {
                        listen_address = config["network"]["listen_address"].as<std::string>();
                    } catch (std::exception &exception) {
                        std::cerr << "The format of listen_address was incorrect." << std::endl;
                        return false;
                    }

                    if (INADDR_NONE == inet_addr(listen_address.c_str())) {
                        std::cerr << "The format of listen_address was incorrect." << std::endl;
                        return false;
                    }
                }

                if (config["network"]["listen_port"].IsDefined()) {
                    try {
                        listen_port = config["network"]["listen_port"].as<uint16_t>();
                    } catch (std::exception &exception) {
                        std::cerr << "The format of listen_port was incorrect." << std::endl;
                        return false;
                    }
                }
            }

            if (!config["database"].IsDefined()) {
                std::cerr << "There is no database setup in the configuration file." << std::endl;
                return false;
            } else {
                if (!config["database"].IsMap()) {
                    std::cerr << "The format of database was incorrect." << std::endl;
                    return false;
                } else {
                    if (!config["database"]["address"].IsDefined()) {
                        std::cerr << "The address of database was not specific in the configuration." << std::endl;
                        return false;
                    } else {
                        try {
                            database_address = config["database"]["address"].as<std::string>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of database's address was incorrect." << std::endl;
                            return false;
                        }

                        if (INADDR_NONE == inet_addr(database_address.c_str())) {
                            std::cerr << "The format of database's address was incorrect." << std::endl;
                            return false;
                        }
                    }
                    if (!config["database"]["port"].IsDefined()) {
                        std::cerr << "The port of database was not specific in the configuration." << std::endl;
                        return false;
                    } else {
                        try {
                            database_port = config["database"]["port"].as<uint16_t>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of database's port was incorrect." << std::endl;
                            return false;
                        }
                    }
                    if (!config["database"]["dbname"].IsDefined()) {
                        std::cerr << "The dbname of database was not specific in the configuration." << std::endl;
                        return false;
                    } else {
                        try {
                            database_dbname = config["database"]["dbname"].as<std::string>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of database's dbname was incorrect." << std::endl;
                            return false;
                        }
                    }
                    if (!config["database"]["user"].IsDefined()) {
                        std::cerr << "The user for connect to database was not specific in the configuration."
                                  << std::endl;
                        return false;
                    } else {

                        try {
                            database_user = config["database"]["user"].as<std::string>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of database's user was incorrect." << std::endl;
                            return false;
                        }
                    }

                    if (!config["database"]["password"].IsDefined()) {
                        std::cerr << "The password for connect to database was not specific in the configuration."
                                  << std::endl;
                        return false;
                    } else {
                        try {
                            database_password = config["database"]["password"].as<std::string>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of database's password was incorrect." << std::endl;
                            return false;
                        }
                    }
                }
            }

            if (!config["node"].IsDefined()) {
                std::cerr << "There is no node setup in the configuration file." << std::endl;
                return false;
            } else {

                if (!config["node"].IsMap()) {
                    std::cerr << "The format of node was incorrect." << std::endl;
                    return false;
                } else {

                    if (!config["node"]["server_id"].IsDefined()) {
                        std::cerr << "The server_id of node was not specific in the configuration." << std::endl;
                        return false;
                    } else {
                        try {
                            node_server_id = config["node"]["server_id"].as<std::string>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of node's erver_id was incorrect" << std::endl;
                            return false;
                        }
                    }

                    if (!config["node"]["invitation_code"].IsDefined()) {
                        std::cerr << "The invitation_code of node was not specific in the configuration." << std::endl;
                        return false;
                    } else {
                        try {
                            node_invitation_code = config["node"]["invitation_code"].as<std::string>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of node's invitation_code was incorrect" << std::endl;
                            return false;
                        }
                    }

                    if (!config["node"]["president_address"].IsDefined()) {
                        std::cerr << "The president_address was not specific in the configuration." << std::endl;
                        return false;
                    } else {
                        try {
                            president_address = config["node"]["president_address"].as<std::string>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of node's president_address was incorrect" << std::endl;
                            return false;
                        }

                        if (INADDR_NONE == inet_addr(president_address.c_str())) {
                            std::cerr << "The format of node's president_address was incorrect." << std::endl;
                            return false;
                        }
                    }

                    if (config["node"]["president_port"].IsDefined()) {
                        try {
                            president_port = config["node"]["president_port"].as<uint16_t>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of node's president_port was incorrect" << std::endl;
                            return false;
                        }

                    }

                    if (!config["node"]["longitude"].IsDefined()) {
                        std::cerr << "The node's longitude was not specific in the configuration." << std::endl;
                        return false;
                    } else {
                        try {
                            lngLatPair.first = config["node"]["longitude"].as<float>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of node's longitude was incorrect" << std::endl;
                            return false;
                        }

                        if (std::abs(lngLatPair.first) > 180) {
                            std::cerr << "The format of node's longitude was incorrect" << std::endl;
                            return false;
                        }
                    }

                    if (!config["node"]["latitude"].IsDefined()) {
                        std::cerr << "The node's latitude was not specific in the configuration." << std::endl;
                        return false;
                    } else {
                        try {
                            lngLatPair.second = config["node"]["latitude"].as<float>();
                        } catch (std::exception &exception) {
                            std::cerr << "The format of node's latitude was incorrect" << std::endl;
                            return false;
                        }

                        if (std::abs(lngLatPair.second) > 90) {
                            std::cerr << "The format of node's latitude was incorrect" << std::endl;
                            return false;
                        }
                    }
                }
            }

            log4cxx::BasicConfigurator::configure();
            log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getDebug());
//            log4cxx::Logger::getLogger(ClusterModuleLogger)->setLevel(log4cxx::Level::getDebug());
            log4cxx::Logger::getLogger(SessionModuleLogger)->setLevel(log4cxx::Level::getFatal());
//            log4cxx::Logger::getLogger(GroupChatModuleLogger)->setLevel(log4cxx::Level::getError());
            common::KIMDBConfig dbConfig(database_address, database_port, database_dbname, database_user,
                                         database_password);

            //1.初始化组件
            this->mClusterModulePtr = std::make_shared<ClusterModule>(president_address, president_port, node_server_id,
                                                                      node_invitation_code, lngLatPair);
            this->mMessageCenterModulePtr = std::make_shared<MessageCenterModule>(listen_address, listen_port,
                                                                                  std::make_shared<common::KakaIMMessageAdapter>(),6);
            this->mSessionModulePtr = std::make_shared<SessionModule>();
            this->mAuthenticationModulePtr = std::make_shared<AuthenticationModule>();
            this->mOnlineStateModulePtr = std::make_shared<OnlineStateModule>();
            this->mMessageSendServiceModulePtr = std::make_shared<MessageSendServiceModule>();
            this->mRosterModulePtr = std::make_shared<RosterModule>();
            this->mOfflineModulePtr = std::make_shared<OfflineModule>();
            this->mSingleChatModulePtr = std::make_shared<SingleChatModule>();
            this->mGroupChatModulePtr = std::make_shared<GroupChatModule>();
            this->m_task_semaphore = sem_open("kimNodeSemaphore", O_CREAT, 0644, 0);

            this->mAuthenticationModulePtr->setDBConfig(dbConfig);

            //指定ConnectionOperationService
            this->mSessionModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mAuthenticationModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mOnlineStateModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mOfflineModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mRosterModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mSingleChatModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mGroupChatModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mMessageSendServiceModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mClusterModulePtr->setNodeConnectionQueryService(this->mMessageCenterModulePtr);

            //2.配置MessageCenterModule
            this->mMessageCenterModulePtr->addConnectionDelegate(this->mSessionModulePtr.get());
            this->mMessageCenterModulePtr->addConnectionDelegate(this->mAuthenticationModulePtr.get());
            this->mMessageCenterModulePtr->addMessageFilster(this->mSessionModulePtr.get());
            this->mMessageCenterModulePtr->addMessageFilster(this->mAuthenticationModulePtr.get());
            //3.配置ClusterModule

            //4.配置OnlineStateModule
            this->mOnlineStateModulePtr->setQueryUserAccountWithSessionService(mAuthenticationModulePtr);
            this->mOnlineStateModulePtr->setClusterService(mClusterModulePtr);

            //5.配置RosterModule
            this->mRosterModulePtr->setQueryUserAccountWithSessionService(this->mAuthenticationModulePtr);
            this->mRosterModulePtr->setMessageSendService(this->mMessageSendServiceModulePtr);
            this->mRosterModulePtr->setDBConfig(dbConfig);

            //6.配置MessageSendServiceModule
            this->mMessageSendServiceModulePtr->setClusterService(this->mClusterModulePtr);
            this->mMessageSendServiceModulePtr->setQueryConnectionWithSessionService(this->mAuthenticationModulePtr);
            this->mMessageSendServiceModulePtr->setLoginDeviceQueryService(this->mOnlineStateModulePtr);

            //7.配置SingleChatModule
            this->mSingleChatModulePtr->setMessageSendService(this->mMessageSendServiceModulePtr);
            this->mSingleChatModulePtr->setMessageIDGenerateService(this->mClusterModulePtr);
            this->mSingleChatModulePtr->setClusterService(this->mClusterModulePtr);
            this->mSingleChatModulePtr->setLoginDeviceQueryService(this->mOnlineStateModulePtr);
            this->mSingleChatModulePtr->setQueryConnectionWithSessionService(this->mAuthenticationModulePtr);
            this->mSingleChatModulePtr->setQueryUserAccountWithSessionService(this->mAuthenticationModulePtr);
            this->mSingleChatModulePtr->setMessagePersistenceService(this->mOfflineModulePtr);
            this->mSingleChatModulePtr->setUserRelationService(this->mRosterModulePtr);

            //8.配置OfflineModule
            this->mOfflineModulePtr->setQueryUserAccountWithSessionService(this->mAuthenticationModulePtr);
            this->mOfflineModulePtr->setDBConfig(dbConfig);

            //9.配置GroupChatModule
            this->mGroupChatModulePtr->setQueryUserAccountWithSessionService(this->mAuthenticationModulePtr);
            this->mGroupChatModulePtr->setMessageSendService(this->mMessageSendServiceModulePtr);
            this->mGroupChatModulePtr->setClusterService(this->mClusterModulePtr);
            this->mGroupChatModulePtr->setMessagePersistenceService(this->mOfflineModulePtr);
            this->mGroupChatModulePtr->setMessageIDGenerateService(this->mClusterModulePtr);
            this->mGroupChatModulePtr->setLoginDeviceQueryService(this->mOnlineStateModulePtr);
            this->mGroupChatModulePtr->setQueryConnectionWithSessionService(this->mAuthenticationModulePtr);
            this->mGroupChatModulePtr->setDBConfig(dbConfig);

            //10.处理业务消息
            //请求会话ID消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::RequestSessionIDMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::RequestSessionIDMessage &requestSessionIDMessage = *((kakaIM::Node::RequestSessionIDMessage *) &message);
                        this->mSessionModulePtr->addSessionIDRequesMessage(requestSessionIDMessage,
                                                                           connectionIdentifier);
                    });

            //登陆消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::LoginMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::LoginMessage &loginMessage = *((kakaIM::Node::LoginMessage *) &message);
                        this->mAuthenticationModulePtr->addLoginMessage(loginMessage, connectionIdentifier);
                    });

            //注册消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::RegisterMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::RegisterMessage &registerMessage = *((kakaIM::Node::RegisterMessage *) &message);
                        this->mAuthenticationModulePtr->addRegisterMessage(registerMessage, connectionIdentifier);
                    });
            //离线消息
            //this->mMessageCenterModulePtr->setMessageHandler(kakaIM::Node::LogoutMessage::default_instance().GetTypeName(),[this](const ::google::protobuf::Message & message,connectionIdentifier * connectionIdentifier){
            //  std::cout<<"收到一条离线消息"<<std::endl;
            //  kakaIM::Node::LogoutMessage & logoutMessage = *((kakaIM::Node::LogoutMessage*)&message);
            //  this->mSessionModulePtr->addLogoutMessage(logoutMessage,connectionIdentifier);
            //});

            //好友关系请求消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::BuildingRelationshipRequestMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::BuildingRelationshipRequestMessage &buildingRelationshipRequestMessage = *((kakaIM::Node::BuildingRelationshipRequestMessage *) &message);
                        this->mRosterModulePtr->addBuildingRelationshipRequestMessage(
                                buildingRelationshipRequestMessage, connectionIdentifier);
                    });

            //建立好友关系回复消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::BuildingRelationshipAnswerMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::BuildingRelationshipAnswerMessage &buildingRelationshipAnswerMessage = *((kakaIM::Node::BuildingRelationshipAnswerMessage *) &message);
                        this->mRosterModulePtr->addBuildingRelationshipAnswerMessage(buildingRelationshipAnswerMessage,
                                                                                     connectionIdentifier);
                    });

            //拆除好友关系消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::DestroyingRelationshipRequestMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::DestroyingRelationshipRequestMessage &destroyingRelationshipRequestMessage = *((kakaIM::Node::DestroyingRelationshipRequestMessage *) &message);
                        this->mRosterModulePtr->addDestroyingRelationshipRequestMessage(
                                destroyingRelationshipRequestMessage, connectionIdentifier);
                    });

            //请求好友列表消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FriendListRequestMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::FriendListRequestMessage &friendListRequestMessage = *((kakaIM::Node::FriendListRequestMessage *) &message);
                        this->mRosterModulePtr->addFriendListRequestMessage(friendListRequestMessage,
                                                                            connectionIdentifier);
                    });

            //在线消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::OnlineStateMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::OnlineStateMessage &onlineStateMessage = *((kakaIM::Node::OnlineStateMessage *) &message);;
                        this->mOnlineStateModulePtr->addOnlineStateMessage(onlineStateMessage, connectionIdentifier);
                    });
            //聊天消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::ChatMessage &chatMessage = *((kakaIM::Node::ChatMessage *) &message);
                        this->mSingleChatModulePtr->addChatMessage(chatMessage, connectionIdentifier);
                    });

            //拉取离线消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::PullChatMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::PullChatMessage &pullChatMessage = *((kakaIM::Node::PullChatMessage *) &message);
                        this->mOfflineModulePtr->addPullChatMessage(pullChatMessage, connectionIdentifier);
                    });

            //获取用户电子名片消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FetchUserVCardMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::FetchUserVCardMessage &fetchUserVCardMessage = *((kakaIM::Node::FetchUserVCardMessage *) &message);
                        this->mRosterModulePtr->addFetchUserVCardMessage(fetchUserVCardMessage, connectionIdentifier);
                    });

            //更新用户电子名片消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::UpdateUserVCardMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::UpdateUserVCardMessage &updateUserVCardMessage = *((kakaIM::Node::UpdateUserVCardMessage *) &message);
                        this->mRosterModulePtr->addUpdateUserVCardMessage(updateUserVCardMessage, connectionIdentifier);
                    });

            //创建聊天群消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupCreateRequest::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::ChatGroupCreateRequest &chatGroupCreateRequest = *((kakaIM::Node::ChatGroupCreateRequest *) &message);
                        this->mGroupChatModulePtr->addChatGroupCreateRequestMessage(chatGroupCreateRequest,
                                                                                    connectionIdentifier);
                    });

            //解散聊天群消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupDisbandRequest::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::ChatGroupDisbandRequest &chatGroupDisbandRequest = *((kakaIM::Node::ChatGroupDisbandRequest *) &message);
                        this->mGroupChatModulePtr->addChatGroupDisbandRequestMessage(chatGroupDisbandRequest,
                                                                                     connectionIdentifier);
                    });

            //加入聊天群消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupJoinRequest::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::ChatGroupJoinRequest &chatGroupJoinRequest = *((kakaIM::Node::ChatGroupJoinRequest *) &message);
                        this->mGroupChatModulePtr->addChatGroupJoinRequestMessage(chatGroupJoinRequest,
                                                                                  connectionIdentifier);
                    });

            //聊天群申请响应消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupJoinResponse::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::ChatGroupJoinResponse &chatGroupJoinResponse = *((kakaIM::Node::ChatGroupJoinResponse *) &message);
                        this->mGroupChatModulePtr->addChatGroupJoinResponseMessage(chatGroupJoinResponse,
                                                                                   connectionIdentifier);
                    });

            //退出群组消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupQuitRequest::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::ChatGroupQuitRequest &chatGroupQuitRequest = *((kakaIM::Node::ChatGroupQuitRequest *) &message);
                        this->mGroupChatModulePtr->addChatGroupQuitRequestMessage(chatGroupQuitRequest,
                                                                                  connectionIdentifier);
                    });

            //更新群信息消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::UpdateChatGroupInfoRequest::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::UpdateChatGroupInfoRequest &updateChatGroupInfoRequest = *((kakaIM::Node::UpdateChatGroupInfoRequest *) &message);
                        this->mGroupChatModulePtr->addUpdateChatGroupInfoRequestMessage(updateChatGroupInfoRequest,
                                                                                        connectionIdentifier);
                    });

            //获取群信息消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FetchChatGroupInfoRequest::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::FetchChatGroupInfoRequest &fetchChatGroupInfoRequest = *((kakaIM::Node::FetchChatGroupInfoRequest *) &message);
                        this->mGroupChatModulePtr->addFetchChatGroupInfoRequestMessage(fetchChatGroupInfoRequest,
                                                                                       connectionIdentifier);
                    });

            //获取群成员列表消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FetchChatGroupMemberListRequest::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::FetchChatGroupMemberListRequest &fetchChatGroupMemberListRequest = *((kakaIM::Node::FetchChatGroupMemberListRequest *) &message);
                        this->mGroupChatModulePtr->addFetchChatGroupMemberListRequestMessage(
                                fetchChatGroupMemberListRequest, connectionIdentifier);
                    });

            //获取用户所加入的群组消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FetchChatGroupListRequest::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::FetchChatGroupListRequest &fetchChatGroupListRequest = *((kakaIM::Node::FetchChatGroupListRequest *) &message);
                        this->mGroupChatModulePtr->addFetchChatGroupListRequestMessage(fetchChatGroupListRequest,
                                                                                       connectionIdentifier);
                    });

            //群消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::GroupChatMessage::default_instance().GetTypeName(),
                    [this](const ::google::protobuf::Message &message, const std::string connectionIdentifier) {
                        kakaIM::Node::GroupChatMessage &groupChatMessage = *((kakaIM::Node::GroupChatMessage *) &message);
                        this->mGroupChatModulePtr->addGroupChatMessage(groupChatMessage, connectionIdentifier);
                    });
            if (!this->mClusterModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "集群模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mSessionModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "会话模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mAuthenticationModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "认证模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mOnlineStateModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "在线状态模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mMessageSendServiceModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "消息发送模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mOfflineModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "离线模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mRosterModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "花名册模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mSingleChatModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "单聊模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mGroupChatModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "群聊模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mMessageCenterModulePtr->init()) {
                std::cout << "Node::" << __FUNCTION__ << "消息中心模块初始化失败" << std::endl;
                return false;
            }
            return true;
        }

        Node::~Node() {
            sem_close(this->m_task_semaphore);
        }


        int Node::start() {
            this->mClusterModulePtr->start();
            this->mSessionModulePtr->start();
            this->mAuthenticationModulePtr->start();
            this->mOnlineStateModulePtr->start();
            this->mMessageSendServiceModulePtr->start();
            this->mOfflineModulePtr->start();
            this->mRosterModulePtr->start();
            this->mSingleChatModulePtr->start();
            this->mGroupChatModulePtr->start();
            this->mMessageCenterModulePtr->start();
            sem_wait(this->m_task_semaphore);
            return 0;
        }

        void Node::stop() {

        }
    }
}
