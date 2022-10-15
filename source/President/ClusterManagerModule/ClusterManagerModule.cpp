//
// Created by Kakawater on 2018/1/1.
//
#include <sys/eventfd.h>
#include <functional>
#include <iostream>
#include <fcntl.h>
#include <typeinfo>
#include "ClusterManagerModule.h"
#include "../../Common/proto/MessageCluster.pb.h"
#include "../../Common/proto/KakaIMClientPresident.pb.h"
#include "../President.h"
#include "../../Common/util/Date.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace president {

        bool ClusterManagerModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }
            return true;
        }

        void ClusterManagerModule::execute() {
            while (this->m_isStarted) {
                uint64_t count;
                if (0 < read(this->messageEventfd, &count, sizeof(count))) {
                    while (count-- && false == this->messageQueue.empty()) {
                        this->messageQueueMutex.lock();
                        auto pairIt = std::move(this->messageQueue.front());
                        this->messageQueue.pop();
                        this->messageQueueMutex.unlock();
                        if (pairIt.first->GetTypeName() ==
                            president::RequestJoinClusterMessage::default_instance().GetTypeName()) {
                            this->handleRequestJoinClusterMessage(
                                    *(president::RequestJoinClusterMessage *) pairIt.first.get(), pairIt.second);
                        } else if (pairIt.first->GetTypeName() ==
                                   president::HeartBeatMessage::default_instance().GetTypeName()) {
                            this->handleHeartBeatMessage(*(president::HeartBeatMessage *) pairIt.first.get(),
                                                         pairIt.second);
                        }
                    }
                }
            }
        }

        void ClusterManagerModule::handleRequestJoinClusterMessage(
                const RequestJoinClusterMessage &requestJoinClusterMessage, const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, typeid(this).name() << "::" << __FUNCTION__);
            //解析服务器ID和邀请码
            std::string serverId = requestJoinClusterMessage.serverid();
            std::string invitationCode = requestJoinClusterMessage.invitationcode();

            //1.验证邀请码是否正确
            if (false == this->validateInvitationCode(invitationCode)) {//邀请码错误
                ResponseJoinClusterMessage response;
                response.set_result(
                        ResponseJoinClusterMessage_JoinResult::ResponseJoinClusterMessage_JoinResult_Failure);
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, response);
                }
                //connection->shouldClose();
                LOG4CXX_DEBUG(this->logger, typeid(this).name() << "::" << __FUNCTION__<<"邀请码错误");
                return;
            }
            LOG4CXX_DEBUG(this->logger, typeid(this).name() << "::" << __FUNCTION__<<"邀请码正确");
            //2.验证服务器ID
            if (false == this->validateServerID(serverId)) {//服务器ID错误
                ResponseJoinClusterMessage response;
                response.set_result(
                        ResponseJoinClusterMessage_JoinResult::ResponseJoinClusterMessage_JoinResult_Failure);
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, response);
                }
                //connection->shouldClose();
                LOG4CXX_DEBUG(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 服务器ID错误");
                return;
            }
            LOG4CXX_DEBUG(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 服务器ID正确");
            //3.邀请码和服务器ID都通过，则加入集群

            auto connectionOperationService = this->connectionOperationServicePtr.lock();

            if (!connectionOperationService) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "connectionOperationService不存在");
                return;
            }


            auto pair = connectionOperationService->queryConnectionAddress(connectionIdentifier);
            if (!pair.first) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "查询连接地址失败");
                return;
            }
            this->nodeConnection.emplace(serverId, Node(serverId, connectionIdentifier, pair.second.first,
                                                                      pair.second.second, std::make_pair(
                            requestJoinClusterMessage.longitude(), requestJoinClusterMessage.latitude()),requestJoinClusterMessage.serviceaddr(),requestJoinClusterMessage.serviceport()));
            ResponseJoinClusterMessage response;
            response.set_result(ResponseJoinClusterMessage_JoinResult::ResponseJoinClusterMessage_JoinResult_Success);
            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, response);
            }
            LOG4CXX_INFO(this->logger,__FUNCTION__<<" 服务器"<<serverId<<"加入集群");
            //4.通知其余组件，有新的服务器加入集群
            triggerEvent(ClusterEvent(serverId, ClusterEvent::NewNodeJoinedCluster));
        }

        void ClusterManagerModule::handleHeartBeatMessage(const HeartBeatMessage &message,
                                                          const std::string connectionIdentifier) {
            ResponseHeartBeatMessage responseHeartBeatMessage;
            responseHeartBeatMessage.set_timestamp(kaka::Date::getCurrentDate().toString());
            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                         responseHeartBeatMessage);
            }
        }

        bool ClusterManagerModule::validateInvitationCode(std::string invitationCode) {
            if (invitationCode == this->invitation_code) {
                return true;
            } else {
                return false;
            }
        }

        bool ClusterManagerModule::validateServerID(std::string serverID) {
            auto it = this->nodeConnection.find(serverID);
            if (it != this->nodeConnection.end()) {
                return false;
            } else {
                return true;
            }
        }

        bool ClusterManagerModule::doFilter(const ::google::protobuf::Message &message,
                                            const std::string connectionIdentifier) {
            std::string messageType = message.GetTypeName();
            if (messageType == RequestJoinClusterMessage::default_instance().GetTypeName() || messageType == RequestNodeMessage::default_instance().GetTypeName()) {
                return true;
            }

            //1.判定connection是否已经通过认证
            auto it = std::find_if(this->nodeConnection.begin(), this->nodeConnection.end(), [connectionIdentifier](
                    const std::map<std::string, Node>::value_type &pair) -> bool {
                return connectionIdentifier == pair.second.getServerConnectionIdentifier();
            });
            if (it == this->nodeConnection.end()) {//未通过认证的连接
                return false;
            }

            return true;
        }

        void ClusterManagerModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        std::set<Node> ClusterManagerModule::getAllNodes() {
            std::set<Node> nodeSet;
            for (auto itemIt = this->nodeConnection.begin(); itemIt != this->nodeConnection.end(); ++itemIt) {
                nodeSet.emplace(itemIt->second);
            }
            return nodeSet;
        }

        Node ClusterManagerModule::getNode(std::string serverID) throw(NodeNotExitException) {
            auto nodeIt = this->nodeConnection.find(serverID);
            if (nodeIt != this->nodeConnection.end()) {
                return nodeIt->second;
            } else {
                throw NodeNotExitException(serverID);
            }

        }

        Node ClusterManagerModule::getNodeWithConnectionIdentifier(
                std::string connectionIdentifier)throw(NodeNotExitException) {
            for (auto itemIt = this->nodeConnection.begin(); itemIt != this->nodeConnection.end(); ++itemIt) {
                if (connectionIdentifier == itemIt->second.getServerConnectionIdentifier()) {
                    return itemIt->second;
                }
            }
            throw NodeNotExitException(connectionIdentifier);
        }

        void ClusterManagerModule::addEventListener(ClusterEvent::ClusterEventType eventType,
                                                    std::function<void(ClusterEvent)> callback) {
            this->eventCallbacks.emplace(eventType, callback);
        }

        void ClusterManagerModule::didCloseConnection(const std::string connectionIdentifier) {
            auto it = std::find_if(this->nodeConnection.begin(), this->nodeConnection.end(), [connectionIdentifier](
                    const std::map<std::string, Node>::value_type &pair) -> bool {
                return connectionIdentifier == pair.second.getServerConnectionIdentifier();
            });
            if (it != this->nodeConnection.end()) {
                std::string serverID = it->first;
                this->nodeConnection.erase(it);
                //触发相关关闭事件
                ClusterEvent event(serverID, ClusterEvent::NodeRemovedCluster);
                triggerEvent(event);
                //向集群中的其余节点推送此节点脱离信息
                LOG4CXX_DEBUG(this->logger,__FUNCTION__<<"正在向集群其余节点推送此节点脱离信息")
                NodeSecessionMessage nodeSecessionMessage;
                nodeSecessionMessage.set_serverid(serverID);
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()){
                    for (auto pairIt = this->nodeConnection.begin(); pairIt != this->nodeConnection.end();++pairIt){
                        LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" pairIt->first="<<pairIt->first<<" serverID="<<serverID);
                        if(pairIt->first != serverID){
                            LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" 推送....");
                            connectionOperationService->sendMessageThroughConnection(pairIt->second.getServerConnectionIdentifier(),nodeSecessionMessage);
                        }
                    }
                }else{
                    LOG4CXX_ERROR(this->logger,__FUNCTION__<<" 无法向其余节点推送"<<serverID<<"脱离的信息，由于connectionOperationService不存在");
                }
            }
        }

        void ClusterManagerModule::addRequestJoinClusterMessage(std::unique_ptr<RequestJoinClusterMessage> message,
                                                                const std::string connectionIdentifier) {
            if (!message){
                return;
            }
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(message), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));

        }

        void ClusterManagerModule::addHeartBeatMessage(std::unique_ptr<HeartBeatMessage> message,
                                                       const std::string connectionIdentifier) {
            if (!message){
                return;
            }
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(message), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));

        }

        void ClusterManagerModule::triggerEvent(ClusterEvent event) {
            auto it = this->eventCallbacks.find(event.getEventType());
            while (it != this->eventCallbacks.end()) {
                it->second(event);
                it++;
            }
        }

        ClusterManagerModule::ClusterManagerModule(const std::string invitation_code) : invitation_code(invitation_code),messageEventfd(-1) {
            this->logger = log4cxx::Logger::getLogger(ClusterManagerModuleLogger);
        }

        ClusterManagerModule::~ClusterManagerModule() {
            while (false == this->messageQueue.empty()) {
                this->messageQueue.pop();
            }
        }
    }
}

