//
// Created by Kakawater on 2018/1/1.
//

#include <getopt.h>
#include <arpa/inet.h>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <President/President.h>
#include <Common/KakaIMMessageAdapter.h>
#include <President/ClusterManagerModule/ClusterManagerModule.h>
#include <President/ServerRelayModule/ServerRelayModule.h>
#include <President/UserStateManagerModule/UserStateManagerModule.h>
#include <President/MessageIDGenerateModule/MessageIDGenerateModule.h>
#include <President/NodeLoadBlanceModule/NodeLoadBlanceModule.h>
#include <Common/yaml-cpp/include/yaml.h>

namespace kakaIM {
    namespace president {
        President::President() {
        }

        President::~President() {
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

            if (!config["cluster"].IsDefined()) {
                std::cerr << "There is no cluster setup in the configuration file." << std::endl;
                return false;
            }

            if (!config["cluster"].IsMap()) {
                std::cerr << "The format of cluster was incorrect." << std::endl;
                return false;
            } else {
                if (!config["cluster"]["invitation_code"].IsDefined()) {
                    std::cerr << "The invitation_code was not specific in the configuration." << std::endl;
                    return false;
                } else {
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
                                                                                  std::make_shared<common::KakaIMMessageAdapter>(),
                                                                                  0);
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
            this->mMessageCenterModulePtr->registerMessages(this->mClusterManagerModulePtr);
            this->mMessageCenterModulePtr->registerMessages(this->mMessageIDGenerateModulePtr);
            this->mMessageCenterModulePtr->registerMessages(this->mNodeLoadBlanceModulePtr);
            this->mMessageCenterModulePtr->registerMessages(this->mServerRelayModulePtr);
            this->mMessageCenterModulePtr->registerMessages(this->mUserStateManagerModulePtr);

//            //分发事件
            this->mClusterManagerModulePtr->addEventListener(ClusterEvent::ClusterEventType::NewNodeJoinedCluster,
                                                             [this](ClusterEvent event) {
                                                                 this->mUserStateManagerModulePtr->addEvent(event);
                                                             });

            this->mClusterManagerModulePtr->addEventListener(ClusterEvent::ClusterEventType::NodeRemovedCluster,
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


        int President::run() {
            {
                std::unique_lock<std::mutex> lock(this->m_statusMutex);
                if (Status::Stopped != this->m_status) {
                    return -1;
                }
                this->m_status = Status::Starting;
            }

            this->mMessageCenterModulePtr->start();
            this->mClusterManagerModulePtr->start();
            this->mServerRelayModulePtr->start();
            this->mUserStateManagerModulePtr->start();
            this->mMessageIDGenerateModulePtr->start();
            this->mNodeLoadBlanceModulePtr->start();

            std::unique_lock<std::mutex> lock(this->m_statusMutex);
            this->m_status = Status::Started;
            this->m_statusCV.wait(lock, [this]() {
                return Status::Stopped != this->m_status;
            });
            return 0;
        }

        int President::stop() {
            {
                std::unique_lock<std::mutex> lock(this->m_statusMutex);
                if (Status::Started != this->m_status) {
                    return -1;
                }
                this->m_status = Status::Stopping;
            }

            this->mMessageCenterModulePtr->stop();
            this->mClusterManagerModulePtr->stop();
            this->mServerRelayModulePtr->stop();
            this->mUserStateManagerModulePtr->stop();
            this->mMessageIDGenerateModulePtr->stop();
            this->mNodeLoadBlanceModulePtr->stop();

            std::unique_lock<std::mutex> lock(this->m_statusMutex);
            this->m_status = Status::Stopped;
            this->m_statusCV.notify_all();
            return 0;
        }
    }
}


