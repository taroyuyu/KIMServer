//
// Created by Kakawater on 2018/1/26.
//

#include <zconf.h>
#include <Node/MessageSendServiceModule/MessageSendServiceModule.h>
#include <Node/Log/log.h>
#include <Common/proto/MessageCluster.pb.h>

namespace kakaIM {
    namespace node {
        MessageSendServiceModule::MessageSendServiceModule() : KIMNodeModule(MessageSenderServiceModuleLogger){
            this->mMessageHandlerSet[kakaIM::president::SessionMessage::default_instance().GetTypeName()] = [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier){
                this->handleSessionMessage(*((kakaIM::president::SessionMessage *) message.get()));
            };
            this->mMessageHandlerSet["*"] = [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier){
                this->handleMessageForSend(connectionIdentifier, *message.get());
            };
        }

        const std::unordered_set<std::string> MessageSendServiceModule::messageTypes(){
            std::unordered_set<std::string> messageTypeSet;
            for(auto & record : this->mMessageHandlerSet){
                messageTypeSet.insert(record.first);
            }
            return messageTypeSet;
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
                    this->dispatchServerMessage(*message);
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

        void MessageSendServiceModule::dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>,const std::string> & task){
            const auto messageType = task.first->GetTypeName();
            auto it = this->mMessageHandlerSet.find(messageType);
            if (it != this->mMessageHandlerSet.end()){
                it->second(std::move(task.first),task.second);
            }else{
                it = this->mMessageHandlerSet.find("*");
                if (it != this->mMessageHandlerSet.end()){
                    it->second(std::move(task.first),task.second);
                }
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
            this->mTaskQueue.push(std::move(std::make_pair(std::move(messageDuplicate),userAccount)));
        }

        void
        MessageSendServiceModule::sendMessageToSession(const std::string &serverID, const std::string &sessionID,
                                                       const ::google::protobuf::Message &message) {
            std::unique_ptr<kakaIM::president::SessionMessage> sessionMessage{new kakaIM::president::SessionMessage};
            sessionMessage->set_targetserverid(serverID);
            sessionMessage->set_targetsessionid(sessionID);
            sessionMessage->set_messagetype(message.GetTypeName());
            sessionMessage->set_content(message.SerializeAsString());
            //添加到队列中
            this->mTaskQueue.push(std::move(std::make_pair(std::move(sessionMessage),serverID)));
        }

        void MessageSendServiceModule::didReceivedServerMessageFromCluster(
                const kakaIM::president::ServerMessage &serverMessage) {
            this->mServerMessageQueue.push(serverMessage);
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
