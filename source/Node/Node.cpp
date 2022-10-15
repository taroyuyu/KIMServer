//
// Created by Kakawater on 2018/1/7.
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
	    std::string external_address;
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

		if (config["network"]["external_address"].IsDefined()) {
                    try {
                        external_address = config["network"]["external_address"].as<std::string>();
                    } catch (std::exception &exception) {
                        std::cerr << "The format of external_address was incorrect." << std::endl;
                        return false;
                    }

                    if (INADDR_NONE == inet_addr(external_address.c_str())) {
                        std::cerr << "The format of external_address was incorrect." << std::endl;
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

	    std::string serviceAddr = listen_address;
            if (external_address.length()){
                serviceAddr = external_address;
            }
            //1.初始化组件
            this->mClusterModulePtr = std::make_shared<ClusterModule>(president_address, president_port, node_server_id,
                                                                      node_invitation_code, lngLatPair,serviceAddr,listen_port);
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
            this->mOnlineStateModulePtr->setUserRelationService(mRosterModulePtr);
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
            this->mSingleChatModulePtr->setDBConfig(dbConfig);

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
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSessionModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::RequestSessionIDMessage>((kakaIM::Node::RequestSessionIDMessage*)message.get())),connectionIdentifier);
    			message.release();
                    });

            //登陆消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::LoginMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mAuthenticationModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::LoginMessage>((kakaIM::Node::LoginMessage*)message.get())), connectionIdentifier);
			message.release();
                    });

            //注册消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::RegisterMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mAuthenticationModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::RegisterMessage>((kakaIM::Node::RegisterMessage*)message.get())), connectionIdentifier);
			message.release();
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
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mRosterModulePtr->addMessage(
                                std::move(std::unique_ptr<kakaIM::Node::BuildingRelationshipRequestMessage>((kakaIM::Node::BuildingRelationshipRequestMessage*)message.get())), connectionIdentifier);
			message.release();
                    });

            //建立好友关系回复消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::BuildingRelationshipAnswerMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mRosterModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::BuildingRelationshipAnswerMessage>((kakaIM::Node::BuildingRelationshipAnswerMessage*)message.get())),
                                                                                     connectionIdentifier);
			message.release();
                    });

            //拆除好友关系消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::DestroyingRelationshipRequestMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mRosterModulePtr->addMessage(
                                std::move(std::unique_ptr<kakaIM::Node::DestroyingRelationshipRequestMessage>((kakaIM::Node::DestroyingRelationshipRequestMessage*)message.get())), connectionIdentifier);
			message.release();
                    });

            //请求好友列表消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FriendListRequestMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mRosterModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::FriendListRequestMessage>((kakaIM::Node::FriendListRequestMessage*)message.get())),
                                                                            connectionIdentifier);
			message.release();
                    });

            //在线消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::OnlineStateMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mOnlineStateModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::OnlineStateMessage>((kakaIM::Node::OnlineStateMessage*)message.get())), connectionIdentifier);
			message.release();
                    });
            //聊天消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::ChatMessage>((kakaIM::Node::ChatMessage*)message.get())), connectionIdentifier);
			message.release();
                    });

	    //视频通话请求消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::VideoChatRequestMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::VideoChatRequestMessage>((kakaIM::Node::VideoChatRequestMessage*)message.get())), connectionIdentifier);
                        message.release();
                    });

	    //视频通话请求取消消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::VideoChatRequestCancelMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::VideoChatRequestCancelMessage>((kakaIM::Node::VideoChatRequestCancelMessage*)message.get())), connectionIdentifier);
                        message.release();
                    });
            //视频通话回复消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::VideoChatReplyMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::VideoChatReplyMessage>((kakaIM::Node::VideoChatReplyMessage*)message.get())), connectionIdentifier);
                        message.release();
                    });

            //拉取好友状态消息
            this->mMessageCenterModulePtr->setMessageHandler(kakaIM::Node::PullFriendOnlineStateMessage::default_instance().GetTypeName(),[this](std::unique_ptr<::google::protobuf::Message> message,const std::string connectionIdentifier){
                this->mOnlineStateModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::PullFriendOnlineStateMessage>((kakaIM::Node::PullFriendOnlineStateMessage*)message.get())),connectionIdentifier);
		message.release();
            });

            //视频聊天提议消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::VideoChatOfferMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::VideoChatOfferMessage>((kakaIM::Node::VideoChatOfferMessage*)message.get())), connectionIdentifier);
                        message.release();
                    });            
            //视频聊天应答消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::VideoChatAnswerMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::VideoChatAnswerMessage>((kakaIM::Node::VideoChatAnswerMessage*)message.get())), connectionIdentifier);
                        message.release();
                    });
	    //视频聊天协商响应消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::VideoChatNegotiationResultMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::VideoChatNegotiationResultMessage>((kakaIM::Node::VideoChatNegotiationResultMessage*)message.get())), connectionIdentifier);
                        message.release();
                    });
            //候选项消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::VideoChatCandidateAddressMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::VideoChatCandidateAddressMessage>((kakaIM::Node::VideoChatCandidateAddressMessage*)message.get())), connectionIdentifier);
                        message.release();
                    });
            //视频聊天结束消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::VideoChatByeMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mSingleChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::VideoChatByeMessage>((kakaIM::Node::VideoChatByeMessage*)message.get())), connectionIdentifier);
                        message.release();
                    });


            //拉取离线消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::PullChatMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mOfflineModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::PullChatMessage>((kakaIM::Node::PullChatMessage*)message.get())), connectionIdentifier);
			message.release();
                    });

            //获取用户电子名片消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FetchUserVCardMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mRosterModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::FetchUserVCardMessage>((kakaIM::Node::FetchUserVCardMessage*)message.get())), connectionIdentifier);
			message.release();
                    });

            //更新用户电子名片消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::UpdateUserVCardMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mRosterModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::UpdateUserVCardMessage>((kakaIM::Node::UpdateUserVCardMessage*)message.get())), connectionIdentifier);
			message.release();
                    });

            //创建聊天群消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupCreateRequest::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::ChatGroupCreateRequest>((kakaIM::Node::ChatGroupCreateRequest*)message.get())),
                                                                                    connectionIdentifier);
			message.release();
                    });

            //解散聊天群消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupDisbandRequest::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::ChatGroupDisbandRequest>((kakaIM::Node::ChatGroupDisbandRequest*)message.get())),
                                                                                     connectionIdentifier);
			message.release();
                    });

            //加入聊天群消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupJoinRequest::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
			std::cout<<"收到一条加入聊天群申请"<<std::endl;
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::ChatGroupJoinRequest>((kakaIM::Node::ChatGroupJoinRequest*)message.get())),
                                                                                  connectionIdentifier);
			message.release();
                    });

            //聊天群申请响应消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupJoinResponse::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::ChatGroupJoinResponse>((kakaIM::Node::ChatGroupJoinResponse*)message.get())),
                                                                                   connectionIdentifier);
			message.release();
                    });

            //退出群组消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::ChatGroupQuitRequest::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::ChatGroupQuitRequest>((kakaIM::Node::ChatGroupQuitRequest*)message.get())),
                                                                                  connectionIdentifier);
			message.release();
                    });

            //更新群信息消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::UpdateChatGroupInfoRequest::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::UpdateChatGroupInfoRequest>((kakaIM::Node::UpdateChatGroupInfoRequest*)message.get())),
                                                                                        connectionIdentifier);
			message.release();
                    });

            //获取群信息消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FetchChatGroupInfoRequest::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::FetchChatGroupInfoRequest>((kakaIM::Node::FetchChatGroupInfoRequest*)message.get())),
                                                                                       connectionIdentifier);
			message.release();
                    });

            //获取群成员列表消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FetchChatGroupMemberListRequest::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(
                                std::move(std::unique_ptr<kakaIM::Node::FetchChatGroupMemberListRequest>((kakaIM::Node::FetchChatGroupMemberListRequest*)message.get())), connectionIdentifier);
			message.release();
                    });

            //获取用户所加入的群组消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::FetchChatGroupListRequest::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::FetchChatGroupListRequest>((kakaIM::Node::FetchChatGroupListRequest*)message.get())),
                                                                                       connectionIdentifier);
			message.release();
                    });

            //群消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    kakaIM::Node::GroupChatMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
                        this->mGroupChatModulePtr->addMessage(std::move(std::unique_ptr<kakaIM::Node::GroupChatMessage>((kakaIM::Node::GroupChatMessage*)message.get())), connectionIdentifier);
			message.release();
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
