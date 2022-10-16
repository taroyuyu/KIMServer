//
// Created by Kakawater on 2018/1/26.
//

#include <zconf.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "MessageSendServiceModule.h"
#include "../Log/log.h"
#include "../../Common/proto/MessageCluster.pb.h"

namespace kakaIM {
    namespace node {
        MessageSendServiceModule::MessageSendServiceModule() : mEpollInstance(-1), messageEventfd(-1),
                                                               serverMessageEventfd(-1) {
            this->logger = log4cxx::Logger::getLogger(MessageSenderServiceModuleLogger);
        }

        MessageSendServiceModule::~MessageSendServiceModule() {
            if (auto clusterService = this->mClusterServicePtr.lock()) {
                clusterService->removeServerMessageListener(this);
            }
        }

        void MessageSendServiceModule::setClusterService(std::weak_ptr<ClusterService> service) {
            if (auto clusterService = this->mClusterServicePtr.lock()) {
                clusterService->removeServerMessageListener(this);
            }
            this->mClusterServicePtr = service;
            if (auto clusterService = this->mClusterServicePtr.lock()) {
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
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (not this->m_needStop) {
                bool needSleep = true;
                if (auto task = this->mTaskQueue.try_pop()) {
                    this->dispatchMessage(*task);
                    needSleep = false;
                }

                if (auto message = this->mServerMessageQueue.try_pop()){
                    this->dispatchServerMessage(**message);
                    needSleep = false;
                }

                if (needSleep){
                    std::this_thread::yield();
                }
            }

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }

        void MessageSendServiceModule::dispatchMessage(std::pair<const std::string, std::unique_ptr<::google::protobuf::Message>> & task){
            auto messageType = task.second->GetTypeName();
            if (messageType ==
                kakaIM::president::SessionMessage::default_instance().GetTypeName()) {
                this->handleSessionMessage(
                        *((kakaIM::president::SessionMessage *) task.second.get()));
            } else {
                this->handleMessageForSend(task.first, *task.second.get());
            }
        }

        void MessageSendServiceModule::dispatchServerMessage(kakaIM::president::ServerMessage & message){
            this->handleServerMessageFromCluster(message);
        }

        void MessageSendServiceModule::sendMessageToUser(const std::string &userAccount,
                                                         const ::google::protobuf::Message &message) {
            std::unique_ptr<::google::protobuf::Message> messageDuplicate(message.New());
            messageDuplicate->CopyFrom(message);
            //添加到队列中
            this->mTaskQueue.push(std::move(std::make_pair(userAccount,std::move(messageDuplicate))));
        }

        void
        MessageSendServiceModule::sendMessageToSession(const std::string &serverID, const std::string &sessionID,
                                                       const ::google::protobuf::Message &message) {
            kakaIM::president::SessionMessage *sessionMessage = new kakaIM::president::SessionMessage();
            sessionMessage->set_targetserverid(serverID);
            sessionMessage->set_targetsessionid(sessionID);
            sessionMessage->set_messagetype(message.GetTypeName());
            sessionMessage->set_content(message.SerializeAsString());
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->mMessageQueueMutex);
            this->mMessageQueue.emplace(std::make_pair(serverID, sessionMessage));
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void MessageSendServiceModule::didReceivedServerMessageFromCluster(
                const kakaIM::president::ServerMessage &serverMessage) {
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
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << " 无法工作，由于缺少loginDeviceQueryService或者queryConnectionWithSessionService再或者clusterService")
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
                                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                                             message);
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

        void MessageSendServiceModule::handleSessionMessage(kakaIM::president::SessionMessage &sessionMessage) {
            auto clusterService = this->mClusterServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            if (!(queryConnectionWithSessionService && connectionOperationService && clusterService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << " 无法工作，由于缺少connectionOperationService或者clusterService、queryConnectionWithSessionService")
                return;
            }

            if (sessionMessage.targetserverid() == clusterService->getServerID()) {
                auto descriptor = ::google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
                        sessionMessage.GetTypeName());
                if (nullptr == descriptor) {//不存在此消息类型
                    return;
                }
                ::google::protobuf::Message *message = ::google::protobuf::MessageFactory::generated_factory()->GetPrototype(
                        descriptor)->New();
                message->ParseFromString(sessionMessage.content());
                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                        sessionMessage.targetsessionid());
                if (!deviceConnectionIdentifier.empty()) {
                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier, *message);
                }
            } else {
                kakaIM::president::ServerMessage serverMessage;
                serverMessage.set_messagetype(sessionMessage.GetTypeName());
                serverMessage.set_content(sessionMessage.SerializeAsString());
                serverMessage.set_targetuser(sessionMessage.targetserverid());
                clusterService->sendServerMessage(serverMessage);
            }
        }

/*8
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
*/
        void MessageSendServiceModule::handleServerMessageFromCluster(
                const kakaIM::president::ServerMessage &serverMessage) {
            auto clusterService = this->mClusterServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto loginDeviceQueryService = this->mLoginDeviceQueryServicePtr.lock();
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            if (!(clusterService && connectionOperationService && loginDeviceQueryService &&
                  queryConnectionWithSessionService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << " 无法工作，由于缺少clusterService、connectionOperationService、loginDeviceQueryService、或者、ueryConnectionWithSessionService")
                return;
            }
            //1.根据messageType创建消息，并进行反序列化
            const std::string messageType = serverMessage.messagetype();
            auto descriptor = ::google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(messageType);
            if (nullptr == descriptor) {//不存在此消息类型
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " 无法对" << messageType << "类型的消息进行反序列化");
                return;
            }

            if (messageType == kakaIM::president::SessionMessage::default_instance().GetTypeName()) {
                kakaIM::president::SessionMessage sessionMessage;
                sessionMessage.ParseFromString(serverMessage.content());
                if (sessionMessage.targetserverid() != clusterService->getServerID()) {
                    return;
                }
                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                        sessionMessage.targetsessionid());
                if (!deviceConnectionIdentifier.empty()) {
                    auto descriptor = ::google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
                            sessionMessage.messagetype());
                    std::unique_ptr<::google::protobuf::Message> message(
                            ::google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor)->New());
                    message->ParseFromString(sessionMessage.content());
                    message->GetReflection()->SetString(message.get(),
                                                        message->GetDescriptor()->FindFieldByName("sessionID"),
                                                        sessionMessage.targetsessionid());
                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                             *message.get());
                }
            } else {

                std::unique_ptr<::google::protobuf::Message> message(
                        ::google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor)->New());
                message->ParseFromString(serverMessage.content());
                if (!message->IsInitialized()) {
                    LOG4CXX_ERROR(this->logger, __FUNCTION__ << " " << messageType << "消息 未初始化完成");
                    return;
                }

                //2.根据targetUser,将消息发送给位于当前Node上的用户
                const std::string targetUser = serverMessage.targetuser();
                if (!loginDeviceQueryService || !queryConnectionWithSessionService) {
                    LOG4CXX_ERROR(this->logger, __FUNCTION__
                            << " 无法工作，由于缺少loginDeviceQueryService或者queryConnectionWithSessionService")
                    return;
                }

                auto itPair = loginDeviceQueryService->queryLoginDeviceSetWithUserAccount(targetUser);

                if (itPair.first != itPair.second) {

                    for (auto loginDeviceIt = itPair.first; loginDeviceIt != itPair.second; ++loginDeviceIt) {
                        //1.发送消息
                        if (kakaIM::Node::OnlineStateMessage_OnlineState_Online == loginDeviceIt->second ||
                            kakaIM::Node::OnlineStateMessage_OnlineState_Invisible || loginDeviceIt->second) {
                            if (loginDeviceIt->first.first ==
                                LoginDeviceQueryService::IDType_SessionID) {//设备是在此Server上登陆的

                                const std::string sessionID = loginDeviceIt->first.second;
                                auto fieldPtr = message->GetDescriptor()->FindFieldByName("sessionID");
                                if (fieldPtr) {
                                    message->GetReflection()->SetString(message.get(), fieldPtr, sessionID);
                                }
                                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                                        sessionID);
                                if (!deviceConnectionIdentifier.empty()) {
                                    connectionOperationService->sendMessageThroughConnection(
                                            deviceConnectionIdentifier, *message.get());
                                }
                            }
                        }
                    }
                }
            }
        }

    }
}
