//
// Created by Kakawater on 2018/1/1.
//

#include <fcntl.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include "President.h"
#include "../Common/KakaIMMessageAdapter.h"
#include "ClusterManagerModule/ClusterManagerModule.h"
#include "ServerRelayModule/ServerRelayModule.h"
#include "UserStateManagerModule/UserStateManagerModule.h"
#include "MessageIDGenerateModule/MessageIDGenerateModule.h"
#include "NodeLoadBlanceModule/NodeLoadBlanceModule.h"
#include "../Common/yaml-cpp/include/yaml.h"
#include "Log/log.h"
#include <iostream>

namespace kakaIM {
    namespace president {
        President::President() :
                m_task_semaphore(nullptr) {
            this->m_task_semaphore = sem_open("/kimPresidentSemaphore", O_CREAT, 0644, 0);
        }

        President::~President() {
            sem_close(this->m_task_semaphore);
        }

        bool President::init(int argc, char *argv[]) {
            std::string listen_address;
            uint16_t listen_port = 1221;
            std::string invitationCode;

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

            if (!config["cluster"].IsDefined()){
                std::cerr << "There is no cluster setup in the configuration file." << std::endl;
                return false;
            }
            
            if(!config["cluster"].IsMap()){
                std::cerr << "The format of cluster was incorrect." << std::endl;
                return false;
            }else{
                if(!config["cluster"]["invitation_code"].IsDefined()){
                    std::cerr << "The invitation_code was not specific in the configuration." << std::endl;
                    return false;
                }else{
                    try {
                        invitationCode = config["cluster"]["invitation_code"].as<std::string>();
                    } catch (std::exception &exception) {
                        std::cerr << "The format of invitationCode was incorrect." << std::endl;
                        return false;
                    }
                }
            }


            log4cxx::BasicConfigurator::configure();
            log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getTrace());

            //初始化组件
            this->mMessageCenterModulePtr = std::make_shared<MessageCenterModule>(listen_address, listen_port,
                                                                                  std::make_shared<common::KakaIMMessageAdapter>(),0);
            this->mClusterManagerModulePtr = std::make_shared<ClusterManagerModule>(invitationCode);
            this->mUserStateManagerModulePtr = std::make_shared<UserStateManagerModule>();
            this->mMessageIDGenerateModulePtr = std::make_shared<MessageIDGenerateModule>();
            this->mServerRelayModulePtr = std::make_shared<ServerRelayModule>();
            this->mNodeLoadBlanceModulePtr = std::make_shared<NodeLoadBlanceModule>();

            //配置MessageCenterModule
            this->mMessageCenterModulePtr->addConnectionDelegate(this->mClusterManagerModulePtr.get());
            this->mMessageCenterModulePtr->addMessageFilster(this->mClusterManagerModulePtr.get());

            this->mClusterManagerModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mMessageIDGenerateModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mServerRelayModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mUserStateManagerModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);

            //配置UserStateManagerModule
            this->mUserStateManagerModulePtr->setServerManageService(this->mClusterManagerModulePtr);

            //配置NodeLoadBlanceModule
            this->mNodeLoadBlanceModulePtr->setConnectionOperationService(this->mMessageCenterModulePtr);
            this->mNodeLoadBlanceModulePtr->setServerManageService(this->mClusterManagerModulePtr);
            this->mNodeLoadBlanceModulePtr->setUserStateManagerService(this->mUserStateManagerModulePtr);

            //配置ServerRelayModule
            this->mServerRelayModulePtr->setUserStateManagerService(this->mUserStateManagerModulePtr);
            this->mServerRelayModulePtr->setServerManageService(this->mClusterManagerModulePtr);

            //处理消息
            this->mMessageCenterModulePtr->setMessageHandler(
                    RequestJoinClusterMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
			this->mClusterManagerModulePtr->addRequestJoinClusterMessage(std::move(std::unique_ptr<RequestJoinClusterMessage>((RequestJoinClusterMessage*)message.get())),
                                                                           connectionIdentifier);
                        message.release();
                    });
            this->mMessageCenterModulePtr->setMessageHandler(HeartBeatMessage::default_instance().GetTypeName(),
                                                             [this](std::unique_ptr<::google::protobuf::Message> message,
                                                                    const std::string connectionIdentifier) {
								    this->mClusterManagerModulePtr->addHeartBeatMessage(std::move(std::unique_ptr<HeartBeatMessage>((HeartBeatMessage*)message.get())),
                                                                                                                              connectionIdentifier);
                        				            message.release();
                                                             });
            this->mMessageCenterModulePtr->setMessageHandler(UserOnlineStateMessage::default_instance().GetTypeName(),
                                                             [this](std::unique_ptr<::google::protobuf::Message> message,
                                                                    const std::string connectionIdentifier) {
								    this->mUserStateManagerModulePtr->addUserOnlineStateMessage(std::move(std::unique_ptr<UserOnlineStateMessage>((UserOnlineStateMessage*)message.get())),
                                                                                                                     connectionIdentifier);
                        				 	    message.release();
                                                             });

            this->mMessageCenterModulePtr->setMessageHandler(
                    UpdateUserOnlineStateMessage::default_instance().GetTypeName(),
                    [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier) {
			this->mUserStateManagerModulePtr->addUpdateUserOnlineStateMessage(std::move(std::unique_ptr<UpdateUserOnlineStateMessage>((UpdateUserOnlineStateMessage*)message.get())),
                                                                                    connectionIdentifier);
                        message.release();
                    });

            this->mMessageCenterModulePtr->setMessageHandler(RequestMessageIDMessage::default_instance().GetTypeName(),
                                                             [this](std::unique_ptr<::google::protobuf::Message> message,
                                                                    const std::string connectionIdentifier) {
								    this->mMessageIDGenerateModulePtr->addRequestMessageIDMessage(std::move(std::unique_ptr<RequestMessageIDMessage>((RequestMessageIDMessage*)message.get())),
                                                                                                                                   connectionIdentifier);
	                        			 	    message.release();
                                                             });

            this->mMessageCenterModulePtr->setMessageHandler(ServerMessage::default_instance().GetTypeName(),
                                                             [this](std::unique_ptr<::google::protobuf::Message> message,
                                                                    const std::string connectionIdentifier) {
								    this->mServerRelayModulePtr->addServerMessage(std::move(std::unique_ptr<ServerMessage>((ServerMessage*)message.get())),
                                                                                                                               connectionIdentifier);
                       						     message.release();
                                                             });

            this->mMessageCenterModulePtr->setMessageHandler(NodeLoadInfoMessage::default_instance().GetTypeName(),
                                                             [this](std::unique_ptr<::google::protobuf::Message> message,
                                                                    const std::string connectionIdentifier) {
							            this->mNodeLoadBlanceModulePtr->addNodeLoadInfoMessage(std::move(std::unique_ptr<NodeLoadInfoMessage>((NodeLoadInfoMessage*)message.get())),
                                                                                                               connectionIdentifier);
                        					    message.release();
                                                             });

            this->mMessageCenterModulePtr->setMessageHandler(RequestNodeMessage::default_instance().GetTypeName(),
                                                             [this](std::unique_ptr<::google::protobuf::Message> message,
                                                                    const std::string connectionIdentifier) {
								    this->mNodeLoadBlanceModulePtr->addRequestNodeMessage(std::move(std::unique_ptr<RequestNodeMessage>((RequestNodeMessage*)message.get())),
                                                                                                                        connectionIdentifier);
								    message.release();
                                                             });

//            //分发事件
            this->mClusterManagerModulePtr->addEventListener(ClusterEvent::NewNodeJoinedCluster,
                                                             [this](ClusterEvent event) {
                                                                 this->mUserStateManagerModulePtr->addEvent(event);
                                                             });

            this->mClusterManagerModulePtr->addEventListener(ClusterEvent::NodeRemovedCluster,
                                                             [this](ClusterEvent event) {
                                                                 this->mUserStateManagerModulePtr->addEvent(event);
                                                             });

            if (!this->mClusterManagerModulePtr->init()) {
                std::cerr << "President::" << __FUNCTION__ << "集群管理模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mUserStateManagerModulePtr->init()) {
                std::cerr << "President::" << __FUNCTION__ << "用户在线状态管理模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mMessageIDGenerateModulePtr->init()) {
                std::cerr << "President::" << __FUNCTION__ << "消息ID生成器模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mServerRelayModulePtr->init()) {
                std::cerr << "President::" << __FUNCTION__ << "消息转发模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mNodeLoadBlanceModulePtr->init()) {
                std::cerr << "president::" << __FUNCTION__ << "节点均衡模块初始化失败" << std::endl;
                return false;
            }
            if (!this->mMessageCenterModulePtr->init()) {
                std::cerr << "President::" << __FUNCTION__ << "消息中心模块初始化失败" << std::endl;
                return false;
            }
            return true;
        }


        int President::start() {
            this->mMessageCenterModulePtr->start();
            this->mClusterManagerModulePtr->start();
            this->mServerRelayModulePtr->start();
            this->mUserStateManagerModulePtr->start();
            this->mMessageIDGenerateModulePtr->start();
            this->mNodeLoadBlanceModulePtr->start();
            sem_wait(this->m_task_semaphore);
            return 0;
        }
    }
}


