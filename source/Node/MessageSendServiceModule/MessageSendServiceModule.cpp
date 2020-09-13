//
// Created by taroyuyu on 2018/1/26.
//

#include <zconf.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "MessageSendServiceModule.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace node {
        MessageSendServiceModule::MessageSendServiceModule() :mEpollInstance(-1),messageEventfd(-1),serverMessageEventfd(-1) {
	    this->logger = log4cxx::Logger::getLogger(MessageSenderServiceModuleLogger);
        }

        MessageSendServiceModule::~MessageSendServiceModule() {
            if(auto clusterService = this->mClusterServicePtr.lock()){
                clusterService->removeServerMessageListener(this);
            }
        }

        void MessageSendServiceModule::setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr){
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void MessageSendServiceModule::setLoginDeviceQueryService(std::weak_ptr<LoginDeviceQueryService> service) {
            this->mLoginDeviceQueryServicePtr = service;
        }

        void MessageSendServiceModule::setQueryConnectionWithSessionService(
                std::weak_ptr<QueryConnectionWithSession> service) {
            this->mQueryConnectionWithSessionServicePtr = service;
        }

        void MessageSendServiceModule::setClusterService(std::weak_ptr<ClusterService> service) {
            if(auto clusterService = this->mClusterServicePtr.lock()){
                clusterService->removeServerMessageListener(this);
            }
            this->mClusterServicePtr = service;
            if(auto clusterService = this->mClusterServicePtr.lock()){
                clusterService->addServerMessageListener(this);
            }
        }

        bool MessageSendServiceModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }

            this->serverMessageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->serverMessageEventfd < 0) {
                return false;
            }

            //创建Epoll实例
            if (-1 == (this->mEpollInstance = epoll_create1(0))) {
                return false;
            }

            //向Epill实例注册messageEventfd,clusterMessageEventfd
            struct epoll_event messageEventfdEvent;
            messageEventfdEvent.events = EPOLLIN;
            messageEventfdEvent.data.fd = this->messageEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->messageEventfd, &messageEventfdEvent)) {
                return false;
            }

            struct epoll_event serverMessageEvent;
            serverMessageEvent.events = EPOLLIN;
            serverMessageEvent.data.fd = this->serverMessageEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->serverMessageEventfd, &serverMessageEvent)) {
                return false;
            }
            return true;
        }

        void MessageSendServiceModule::execute() {
            while (this->m_isStarted) {
                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为0.1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   1000);

                if (-1 == happedEventsCount) {
                    LOG4CXX_WARN(this->logger,__FUNCTION__<<" 等待Epil实例上的事件出错，errno ="<<errno);
                }
                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (EPOLLIN & happedEvents[i].events) {
                        if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < ::read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->mMessageQueue.empty()) {
                                    this->mMessageQueueMutex.lock();
                                    auto pairIt = std::move(this->mMessageQueue.front());
                                    this->mMessageQueue.pop();
                                    this->mMessageQueueMutex.unlock();

                                    this->handleMessageForSend(pairIt.first, *pairIt.second.get());
                                }

                            } else {
                                LOG4CXX_WARN(this->logger, typeid(this).name()<<""<<__FUNCTION__<<"read(messageEventfd)操作出错，errno ="<<errno);
                            }
                        }else if(this->serverMessageEventfd == happedEvents[i].data.fd){
                            uint64_t count;
                            if (0 < ::read(this->serverMessageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->mServerMessageQueue.empty()) {
                                    this->mServerMessageQueueMutex.lock();
                                    auto serverMessage = std::move(this->mServerMessageQueue.front());
                                    this->mServerMessageQueue.pop();
                                    this->mServerMessageQueueMutex.unlock();
                                    this->handleServerMessageFromCluster(*serverMessage.get());
                                }

                            } else {
                                LOG4CXX_WARN(this->logger, typeid(this).name()<<""<<__FUNCTION__<<"read(messageEventfd)操作出错，errno ="<<errno);
                            }
                        }
                    }
                }
            }
        }

        void MessageSendServiceModule::sendMessageToUser(const std::string &userAccount,
                                                         const ::google::protobuf::Message &message) {
	    std::unique_ptr<::google::protobuf::Message> messageDuplicate(message.New());
            messageDuplicate->CopyFrom(message);
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->mMessageQueueMutex);
            this->mMessageQueue.emplace(std::make_pair(userAccount, std::move(messageDuplicate)));
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void MessageSendServiceModule::didReceivedServerMessageFromCluster(const kakaIM::president::ServerMessage & serverMessage){
            std::lock_guard<std::mutex> lock(this->mServerMessageQueueMutex);
            this->mServerMessageQueue.emplace(new kakaIM::president::ServerMessage(serverMessage));
            uint64_t count = 1;
            //增加信号量
            ::write(this->serverMessageEventfd, &count, sizeof(count));
        }

        void MessageSendServiceModule::handleMessageForSend(const std::string &userAccount,
                                                            ::google::protobuf::Message &message) {
            //根据userAccount查询到其相关的sessionID、serverID
            auto loginDeviceQueryService = this->mLoginDeviceQueryServicePtr.lock();
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            if (!loginDeviceQueryService || !queryConnectionWithSessionService || !clusterService) {
                LOG4CXX_ERROR(this->logger,__FUNCTION__<<" 无法工作，由于缺少loginDeviceQueryService或者queryConnectionWithSessionService再或者clusterService")
                return;
            }

            auto itPair = loginDeviceQueryService->queryLoginDeviceSetWithUserAccount(userAccount);

            if (itPair.first != itPair.second) {

                for (auto loginDeviceIt = itPair.first; loginDeviceIt != itPair.second; ++loginDeviceIt) {
                    //1.发送消息
                    if (kakaIM::Node::OnlineStateMessage_OnlineState_Online == loginDeviceIt->second ||
                        kakaIM::Node::OnlineStateMessage_OnlineState_Invisible || loginDeviceIt->second) {
                        if (loginDeviceIt->first.first == LoginDeviceQueryService::IDType_SessionID) {//设备是在此Server上登陆的

                            const std::string sessionID = loginDeviceIt->first.second;
                            auto fieldPtr = message.GetDescriptor()->FindFieldByName("sessionID");
                            if (fieldPtr) {
                                message.GetReflection()->SetString(&message, fieldPtr, sessionID);
                            }
                            const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                                    sessionID);
                            if (!deviceConnectionIdentifier.empty()) {
                                if(auto connectionOperationService = this->connectionOperationServicePtr.lock()){
                                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,message);
                                }
                            }
                        } else if (loginDeviceIt->first.first ==
                                   LoginDeviceQueryService::IDType_ServerID) {//设备在其它Server上登陆
                            const std::string serverID = loginDeviceIt->first.second;
                            kakaIM::president::ServerMessage serverMessage;
                            serverMessage.set_serverid(serverID);
                            serverMessage.set_messagetype(message.GetTypeName());
                            serverMessage.set_content(message.SerializeAsString());
                            serverMessage.set_targetuser(userAccount);
                            if (serverMessage.IsInitialized()) {
                                clusterService->sendServerMessage(serverMessage);
                            } else {

                            }
                        }
                    }
                }
            }
        }

        void MessageSendServiceModule::handleServerMessageFromCluster(const kakaIM::president::ServerMessage & serverMessage){
            //1.根据messageType创建消息，并进行反序列化
            const std::string messageType = serverMessage.messagetype();
            auto descriptor = ::google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(messageType);
            if(nullptr == descriptor){//不存在此消息类型
                LOG4CXX_ERROR(this->logger,__FUNCTION__<<" 无法对"<<messageType<<"类型的消息进行反序列化");
                return;
            }
            std::unique_ptr<::google::protobuf::Message> message(::google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor)->New());
            message->ParseFromString(serverMessage.content());
            if(!message->IsInitialized()){
                LOG4CXX_ERROR(this->logger,__FUNCTION__<<" "<<messageType<<"消息 未初始化完成");
                return;
            }

            //2.根据targetUser,将消息发送给位于当前Node上的用户
            const std::string targetUser =  serverMessage.targetuser();
            auto loginDeviceQueryService = this->mLoginDeviceQueryServicePtr.lock();
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            if (!loginDeviceQueryService || !queryConnectionWithSessionService) {
                LOG4CXX_ERROR(this->logger,__FUNCTION__<<" 无法工作，由于缺少loginDeviceQueryService或者queryConnectionWithSessionService")
                return;
            }

            auto itPair = loginDeviceQueryService->queryLoginDeviceSetWithUserAccount(targetUser);

            if (itPair.first != itPair.second) {

                for (auto loginDeviceIt = itPair.first; loginDeviceIt != itPair.second; ++loginDeviceIt) {
                    //1.发送消息
                    if (kakaIM::Node::OnlineStateMessage_OnlineState_Online == loginDeviceIt->second ||
                        kakaIM::Node::OnlineStateMessage_OnlineState_Invisible || loginDeviceIt->second) {
                        if (loginDeviceIt->first.first == LoginDeviceQueryService::IDType_SessionID) {//设备是在此Server上登陆的

                            const std::string sessionID = loginDeviceIt->first.second;
                            auto fieldPtr = message->GetDescriptor()->FindFieldByName("sessionID");
                            if (fieldPtr) {
                                message->GetReflection()->SetString(message.get(), fieldPtr, sessionID);
                            }
                            const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                                    sessionID);
                            if (!deviceConnectionIdentifier.empty()) {
                                if(auto connectionOperationService = this->connectionOperationServicePtr.lock()){
                                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,*message.get());
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
