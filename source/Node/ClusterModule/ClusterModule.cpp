//
// Created by Kakawater on 2018/1/7.
//

#include <zconf.h>
#include <iostream>
#include <sys/time.h>
#include <Node/ClusterModule/ClusterModule.h>
#include <Common/util/Date.h>
#include <Common/util/cpu_util.h>
#include <Common/util/mem_util.h>
#include <Common/EventBus/EventBus.h>
#include <Node/Events/NodeSecessionEvent.h>
#include <Node/Events/ClusterDisengagementEvent.h>
#include <Node/Log/log.h>
#include <Common/proto/KakaIMMessage.pb.h>

namespace kakaIM {
    namespace node {
        ClusterModule::ClusterModule(std::string presidentAddr, int presidentPort, std::string serverID,
                                     std::string invitationCode, std::pair<float, float> lngLatPair,
                                     std::string serviceAddr,
                                     uint16_t servicePort) : KIMNodeModule(ClusterModuleLogger),
                                                             mKakaIMMessageAdapter(nullptr),
                                                             mPresidentAddr(presidentAddr),
                                                             mPresidentPort(presidentPort),
                                                             mServerID(serverID),
                                                             mInvitationCode(invitationCode), mlngLatPair(lngLatPair),
                                                             serviceAddr(serviceAddr), servicePort(servicePort){
            this->mKakaIMMessageAdapter = new common::KakaIMMessageAdapter();
        }

        std::string ClusterModule::getServerID() const {
            return this->mServerID;
        }

        ClusterModule::~ClusterModule() {
            this->mHeartBeatTimer.stop();
            if (this->mKakaIMMessageAdapter) {
                delete this->mKakaIMMessageAdapter;
                this->mKakaIMMessageAdapter = nullptr;
            }
        }

        void ClusterModule::execute() {
            this->moduleState = ClusterModuleState::ConnectingPresident;

            // 1.连接President
            mPresidentSocket.connect(this->mPresidentAddr, this->mPresidentPort);
            // 2.将socket加入socketManager
            this->mSocketManager.addSocket(mPresidentSocket, this, this->mKakaIMMessageAdapter);
            // 3.启动socketManager
            this->mSocketManager.start();

            // 4.发送加入集群的请求
            president::RequestJoinClusterMessage *message = new president::RequestJoinClusterMessage();
            message->set_serverid(this->mServerID);
            message->set_invitationcode(this->mInvitationCode);
            message->set_longitude(this->mlngLatPair.first);
            message->set_latitude(this->mlngLatPair.second);
            message->set_serviceaddr(this->serviceAddr);
            message->set_serviceport(this->servicePort);
            this->mSocketManager.sendMessage(mPresidentSocket, *(const ::google::protobuf::Message *) message);
            LOG4CXX_INFO(this->logger, typeid(this).name() << __FUNCTION__ << "发送加入集群的请求");

            // 5. 更新状态
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (not this->m_needStop) {
                if (auto task = this->mMessageQueue.try_pop()) {
                    this->dispatchClusterMessage(*task);
                } else {
                    std::this_thread::yield();
                }
            }

            // 停止定时器
            this->mHeartBeatTimer.stop();
            // 停止socketManager
            this->mSocketManager.stop();

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }

        void ClusterModule::dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task){

        }
        void ClusterModule::dispatchClusterMessage(
                std::pair<std::unique_ptr<::google::protobuf::Message>, MessageSource> &pair) {
            auto messageType = pair.first->GetTypeName();
            if (pair.second == MessageSource::Cluster) {
                if (messageType ==
                    president::ResponseHeartBeatMessage::default_instance().GetTypeName()) {
                    this->handleResponseHeartBeatMessage(
                            *static_cast<const president::ResponseHeartBeatMessage *>(pair.first.get()));
                } else if (messageType ==
                           president::UserOnlineStateMessage::default_instance().GetTypeName()) {
                    this->handleUpdateUserOnlineStateMessageFromCluster(
                            *static_cast<const president::UserOnlineStateMessage *>(pair.first.get()));
                } else if (messageType ==
                           president::UpdateUserOnlineStateMessage::default_instance().GetTypeName()) {
                    this->handleUpdateUserOnlineStateMessageFromCluster(
                            *static_cast<const president::UpdateUserOnlineStateMessage *>(pair.first.get()));
                } else if (messageType ==
                           president::ServerMessage::default_instance().GetTypeName()) {
                    this->handleServerMessageFromCluster(
                            *static_cast<const president::ServerMessage *>(pair.first.get()));
                } else if (messageType ==
                           president::ResponseJoinClusterMessage::default_instance().GetTypeName()) {
                    this->handleResponseJoinClusterMessage(
                            *static_cast<const president::ResponseJoinClusterMessage *>(pair.first.get()));
                } else {
                    LOG4CXX_DEBUG(this->logger,
                                  typeid(this).name() << "" << __FUNCTION__ << "收到一条" << messageType
                                                      << ",但是没有处理函数");
                }
            } else if (pair.second == MessageSource::NodeInternal) {
                if (messageType == Node::OnlineStateMessage::default_instance().GetTypeName()) {
                    this->handleUpdateUserOnlineStateMessageFromNode(
                            *static_cast<Node::OnlineStateMessage *>(pair.first.get()));
                } else if (messageType ==
                           president::ServerMessage::default_instance().GetTypeName()) {
                    this->handleServerMessageFromNode(
                            *static_cast<const president::ServerMessage *>(pair.first.get()));
                } else {
                    LOG4CXX_DEBUG(this->logger,
                                  typeid(this).name() << "" << __FUNCTION__ << "收到一条" << messageType
                                                      << ",但是没有处理函数");
                }
            } else {
                LOG4CXX_DEBUG(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "收到一条未知来源的消息");
            }
        }

        void ClusterModule::handleHeartBeatEvent() {
            //1.获取CPU使用率
            static kaka::CpuUsage cpuUsage_last, cpuUsage_current;
            cpuUsage_last = cpuUsage_current;
            auto pair = kaka::CpuUsage::getCurrentCPUUsage();
            if (pair.first) {
                cpuUsage_last = cpuUsage_current;
                cpuUsage_current = pair.second;
                float cpuUsage = 0;
                if (cpuUsage_current.total - cpuUsage_last.total) {
                    cpuUsage = (cpuUsage_current.work - cpuUsage_last.work) /
                               float(cpuUsage_current.total - cpuUsage_last.total);
                }
                //2.获取当前连接数目
                if (auto nodeConnectionQueryService = this->nodeConnectionQueryServicePtr.lock()) {
                    uint64_t nodeCurrentConnectionCount = nodeConnectionQueryService->queryCurrentNodeConnectionCount();
                    //3.获取内存使用率
                    auto pair = kaka::MemoryUsage::getCurrentMemoryUsage();
                    if (pair.first) {
                        float memUsage = (pair.second.total - pair.second.free - pair.second.cached -
                                          pair.second.buffers) / (float) pair.second.total;
                        LOG4CXX_TRACE(this->logger,
                                      __FUNCTION__ << " 内存状态: total=" << pair.second.total << " free="
                                                   << pair.second.free << " cached=" << pair.second.cached
                                                   << " buffes=" << pair.second.buffers);
                        //4.发送节点负载信息
                        president::NodeLoadInfoMessage nodeLoadInfoMessage;
                        nodeLoadInfoMessage.set_cpuusage(cpuUsage);
                        nodeLoadInfoMessage.set_memusage(memUsage);
                        nodeLoadInfoMessage.set_connectioncount(nodeCurrentConnectionCount);
                        LOG4CXX_TRACE(this->logger,
                                      __FUNCTION__ << "发送节点负载信息,cpu使用率为:" << cpuUsage << " 内存使用率为:"
                                                   << memUsage);
                        this->mSocketManager.sendMessage(this->mPresidentSocket, nodeLoadInfoMessage);
                    } else {
                        LOG4CXX_WARN(this->logger,
                                     typeid(this).name() << "" << __FUNCTION__ << " 获取内存使用率失败，无法发送节点负载信息"
                                                         << errno);
                    }
                } else {
                    LOG4CXX_FATAL(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                    << " nodeConnectionQueryService不存在，无法发送节点负载信息"
                                                                    << errno);
                }
            } else {
                LOG4CXX_WARN(this->logger,
                             typeid(this).name() << "" << __FUNCTION__ << " 获取CPU利用率失败，无法发送节点负载信息"
                                                 << errno);
            }
            //5.发送心跳消息
            president::HeartBeatMessage heartBeatMessage;
            heartBeatMessage.set_serverid(this->mServerID);
            heartBeatMessage.set_timestamp(kaka::Date::getCurrentDate().toString());
            LOG4CXX_TRACE(this->logger, __FUNCTION__ << "发送心跳心跳消息");
            this->mSocketManager.sendMessage(this->mPresidentSocket,
                                             *(const ::google::protobuf::Message *) &heartBeatMessage);
        }

        void ClusterModule::handleResponseJoinClusterMessage(const president::ResponseJoinClusterMessage &message) {
            switch (message.result()) {
                case president::ResponseJoinClusterMessage_JoinResult_Success: {
                    LOG4CXX_INFO(this->logger, __FUNCTION__ << "连接集群成功");
                    this->moduleState = ClusterModuleState::ConnectedPresident;
                    this->mHeartBeatTimer.setInterval([=]() {
                        this->handleHeartBeatEvent();
                    }, std::chrono::seconds(3));
                }
                    break;
                case president::ResponseJoinClusterMessage_JoinResult_Failure: {
                    LOG4CXX_FATAL(this->logger, __FUNCTION__ << " 连接服务器集群失败");
                }
                    break;
            }
        }

        void ClusterModule::handleResponseHeartBeatMessage(const president::ResponseHeartBeatMessage &message) {

        }

        void
        ClusterModule::handleUpdateUserOnlineStateMessageFromCluster(const president::UserOnlineStateMessage &message) {
            for (auto userOnlineStateListenerIt = this->mUserOnlineStateListenerList.begin();
                 userOnlineStateListenerIt != this->mUserOnlineStateListenerList.end(); ++userOnlineStateListenerIt) {
                (*userOnlineStateListenerIt)->didReceivedUserOnlineStateFromCluster(message);
            }
        }

        void ClusterModule::handleUpdateUserOnlineStateMessageFromCluster(
                const president::UpdateUserOnlineStateMessage &message) {
            auto userOnlineStateList = message.useronlinestate();
            for (int i = 0; i < userOnlineStateList.size(); ++i) {
                auto userOnlineState = userOnlineStateList.Get(i);
                this->handleUpdateUserOnlineStateMessageFromCluster(userOnlineState);
            }

        }

        void ClusterModule::updateUserOnlineState(const kakaIM::Node::OnlineStateMessage &message) {
            std::unique_ptr<Node::OnlineStateMessage> onlineStateMessage(new Node::OnlineStateMessage());
            onlineStateMessage->CopyFrom(message);
            this->mMessageQueue.push(std::move(onlineStateMessage), MessageSource::NodeInternal);
        }

        void ClusterModule::addUserOnlineStateListener(UserOnlineStateListener &userOnlineStateListener) {
            auto it = std::find(this->mUserOnlineStateListenerList.begin(), this->mUserOnlineStateListenerList.end(),
                                &userOnlineStateListener);
            if (it == this->mUserOnlineStateListenerList.end()) {
                this->mUserOnlineStateListenerList.emplace_back(&userOnlineStateListener);
            }
        }

        void ClusterModule::removeUserOnlineStateListener(const UserOnlineStateListener &userOnlineStateListener) {
            auto it = std::find(this->mUserOnlineStateListenerList.begin(), this->mUserOnlineStateListenerList.end(),
                                &userOnlineStateListener);
            if (it != this->mUserOnlineStateListenerList.end()) {
                this->mUserOnlineStateListenerList.erase(it);
            }
        }

        void
        ClusterModule::handleUpdateUserOnlineStateMessageFromNode(const kakaIM::Node::OnlineStateMessage &message) {
            LOG4CXX_DEBUG(this->logger, __FUNCTION__);
            president::UserOnlineStateMessage userOnlineStateMessage;
            userOnlineStateMessage.set_serverid(this->mServerID);
            userOnlineStateMessage.set_useraccount(message.useraccount());
            switch (message.userstate()) {
                case Node::OnlineStateMessage_OnlineState_Online: {
                    userOnlineStateMessage.set_userstate(president::UserOnlineStateMessage_OnlineState_Online);
                    LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 用户:" << message.useraccount() << "在线");
                }
                    break;
                case Node::OnlineStateMessage_OnlineState_Invisible: {
                    userOnlineStateMessage.set_userstate(president::UserOnlineStateMessage_OnlineState_Invisible);
                    LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 用户:" << message.useraccount() << "隐身");
                }
                    break;
                case Node::OnlineStateMessage_OnlineState_Offline: {
                    userOnlineStateMessage.set_userstate(president::UserOnlineStateMessage_OnlineState_Offline);
                    LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 用户:" << message.useraccount() << "离线");
                }
                    break;
                default: {
                    return;
                }
            }
            LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 推送给集群");
            this->mSocketManager.sendMessage(this->mPresidentSocket, userOnlineStateMessage);
        }


        void ClusterModule::sendServerMessage(const kakaIM::president::ServerMessage &message) {
            std::unique_ptr<president::ServerMessage> serverMessage(new president::ServerMessage(message));
            serverMessage->set_serverid(this->mServerID);
            this->mMessageQueue.push(std::move(serverMessage), MessageSource::NodeInternal);
        }

        void ClusterModule::addServerMessageListener(ServerMessageListener *listener) {
            std::lock_guard<std::mutex> lock(this->serverMessageListenerSetMutex);
            if (this->serverMessageListenerSet.find(listener) == this->serverMessageListenerSet.end()) {
                this->serverMessageListenerSet.emplace(listener);
            }
        }

        void ClusterModule::removeServerMessageListener(ServerMessageListener *listener) {
            std::lock_guard<std::mutex> lock(this->serverMessageListenerSetMutex);
            this->serverMessageListenerSet.erase(listener);
        }

        void ClusterModule::handleServerMessageFromCluster(const president::ServerMessage &serverMessage) {
            if (serverMessage.serverid() != this->mServerID) {//如果serverID不匹配,则丢弃
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " serverID不匹配，不对其进行处理");
                return;
            }

            auto messageType = serverMessage.messagetype();
            LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " messageType=" << messageType);
            std::lock_guard<std::mutex> lock(this->serverMessageListenerSetMutex);
            for (auto listener: this->serverMessageListenerSet) {
                listener->didReceivedServerMessageFromCluster(serverMessage);
            }
        }

        void ClusterModule::handleServerMessageFromNode(const president::ServerMessage &serverMessage) {
            this->mSocketManager.sendMessage(this->mPresidentSocket, serverMessage);
        }

        void ClusterModule::didConnectionClosed(net::TCPClientSocket clientSocket) {
            if (clientSocket == this->mPresidentSocket) {//与集群断开连接
                //1.停止定时器
                this->mHeartBeatTimer.stop();
                //2.发布集群脱离视线
                std::shared_ptr<const ClusterDisengagementEvent> event(new ClusterDisengagementEvent());
                kakaIM::EventBus::getDefault().Post(event);
                LOG4CXX_FATAL(this->logger, "脱离集群");
            }
        }

        void ClusterModule::didReceivedMessage(int socketfd, const ::google::protobuf::Message &message) {
            std::string messageType = message.GetTypeName();
            if (messageType == kakaIM::president::ResponseHeartBeatMessage::default_instance().GetTypeName()) {
                std::unique_ptr<president::ResponseHeartBeatMessage> responseHeartBeatMessage(
                        new president::ResponseHeartBeatMessage());
                responseHeartBeatMessage->CopyFrom(message);
                this->mMessageQueue.push(std::move(responseHeartBeatMessage), MessageSource::Cluster);
            } else if (messageType == kakaIM::president::UserOnlineStateMessage::default_instance().GetTypeName()) {
                std::unique_ptr<president::UserOnlineStateMessage> userOnlineStateMessage(
                        new president::UserOnlineStateMessage());
                userOnlineStateMessage->CopyFrom(message);
                this->mMessageQueue.push(std::move(userOnlineStateMessage), MessageSource::Cluster);
            } else if (messageType ==
                       kakaIM::president::UpdateUserOnlineStateMessage::default_instance().GetTypeName()) {
                std::unique_ptr<president::UpdateUserOnlineStateMessage> updateUserOnlineStateMessage(
                        new president::UpdateUserOnlineStateMessage());
                updateUserOnlineStateMessage->CopyFrom(message);
                this->mMessageQueue.push(
                        std::move(updateUserOnlineStateMessage), MessageSource::Cluster);
            } else if (messageType == kakaIM::president::ServerMessage::default_instance().GetTypeName()) {
                std::unique_ptr<president::ServerMessage> serverMessage(new president::ServerMessage());
                serverMessage->CopyFrom(message);
                this->mMessageQueue.push(std::move(serverMessage), MessageSource::Cluster);
            } else if (messageType == kakaIM::president::ResponseMessageIDMessage::default_instance().GetTypeName()) {
                const president::ResponseMessageIDMessage &responseMessageIDMessage = *((const president::ResponseMessageIDMessage *) &message);
                auto callBackIt = this->mMessageIDRequestCallback.find(responseMessageIDMessage.requestid());
                if (callBackIt != this->mMessageIDRequestCallback.end()) {
                    callBackIt->second(responseMessageIDMessage);
                    this->mMessageIDRequestCallback.erase(callBackIt);
                } else {
                }
            } else if (message.GetTypeName() ==
                       kakaIM::president::NodeSecessionMessage::default_instance().GetTypeName()) {
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 接收到NodeSecessionMessage");
                const kakaIM::president::NodeSecessionMessage &nodeSecessionMessage = *static_cast<const kakaIM::president::NodeSecessionMessage *>(&message);
                std::shared_ptr<const NodeSecessionEvent> nodeSecessionEvent(
                        new NodeSecessionEvent(nodeSecessionMessage.serverid()));
                kakaIM::EventBus::getDefault().Post(nodeSecessionEvent);
            } else if (message.GetTypeName() ==
                       kakaIM::president::ResponseJoinClusterMessage::default_instance().GetTypeName()) {
                std::unique_ptr<president::ResponseJoinClusterMessage> responseJoinClusterMessage(
                        new president::ResponseJoinClusterMessage());
                LOG4CXX_TRACE(this->logger, typeid(this).name() << __FUNCTION__ << " 收到加入集群的响应");
                responseJoinClusterMessage->CopyFrom(message);
                this->mMessageQueue.push(std::move(responseJoinClusterMessage), MessageSource::Cluster);
            } else {
                LOG4CXX_DEBUG(this->logger, typeid(this).name() << "" << __FUNCTION__ << "收到一条未知来源的消息");
            }
        }

        std::shared_ptr<MessageIDGenerateService::Futrue>
        ClusterModule::generateMessageIDWithUserAccount(const std::string &userAccount) {
            static unsigned long long requestID = 1;
            kakaIM::president::RequestMessageIDMessage requestMessageIDMessage;
            requestMessageIDMessage.set_useraccount(userAccount);
            requestMessageIDMessage.set_serverid(this->mServerID);
            std::string requestIDStr = std::to_string(requestID++);
            requestMessageIDMessage.set_requestid(requestIDStr);
            requestMessageIDMessage.set_useraccount(userAccount);
            requestMessageIDMessage.set_serverid(this->getServerID());
            assert(requestMessageIDMessage.IsInitialized());
            std::shared_ptr<MessageIDGenerateService::Futrue> futurePtr(new MessageIDFutrue());
            this->mMessageIDRequestCallback.emplace(
                    requestIDStr, [futurePtr](kakaIM::president::ResponseMessageIDMessage response) {
                        MessageIDFutrue *futrue = static_cast<MessageIDFutrue *>(futurePtr.get());
                        futrue->setMessageID(response.messageid());
                    });
            this->mSocketManager.sendMessage(this->mPresidentSocket, requestMessageIDMessage);
            return futurePtr;
        }

        ClusterModule::MessageIDFutrue::MessageIDFutrue() : messageID(0), ready(false) {
            pthread_mutex_init(&this->conditionMutex, nullptr);
            pthread_cond_init(&readyCondition, NULL);
        }

        ClusterModule::MessageIDFutrue::~MessageIDFutrue() {
            pthread_mutex_destroy(&this->conditionMutex);
            pthread_cond_destroy(&this->readyCondition);
        }

        void ClusterModule::MessageIDFutrue::setMessageID(uint64_t messageID) {
            pthread_mutex_lock(&this->conditionMutex);
            this->messageID = messageID;
            this->ready = true;
            pthread_cond_broadcast(&(this->readyCondition));
            pthread_mutex_unlock(&this->conditionMutex);
        }

        uint64_t ClusterModule::MessageIDFutrue::getMessageID() {
            pthread_mutex_lock(&this->conditionMutex);
            if (!this->ready) {
                pthread_cond_wait(&this->readyCondition, &this->conditionMutex);
            }

            pthread_mutex_unlock(&(this->conditionMutex));
            return messageID;
        }
    }
}

