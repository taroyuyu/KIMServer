//
// Created by Kakawater on 2018/1/11.
//

#include <zconf.h>
#include <typeinfo>
#include <President/ServerRelayModule/ServerRelayModule.h>
#include <President/Log/log.h>

namespace kakaIM {
    namespace president {
        ServerRelayModule::ServerRelayModule() : KIMPresidentModule(ServerRelayModuleLogger) {
            this->mMessageTypeSet.insert(ServerMessage::default_instance().GetTypeName());
        }

        ServerRelayModule::~ServerRelayModule() {
        }
        const std::unordered_set<std::string> & ServerRelayModule::messageTypes(){
            return this->mMessageTypeSet;
        }
        void ServerRelayModule::execute() {
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

                if (auto event = this->mEventQueue.try_pop()){
                    this->dispatchClusterEvent(*event);
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

        void ServerRelayModule::dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task){
            this->handleServerMessage(dynamic_cast<ServerMessage&>(*task.first), task.second);
        }

        void ServerRelayModule::dispatchClusterEvent(ClusterEvent & event){
            switch (event.getEventType()) {
                case ClusterEvent::ClusterEventType::NewNodeJoinedCluster: {
                    this->handleNewNodeJoinedClusterEvent(event);
                }
                    break;
                case ClusterEvent::ClusterEventType::NodeRemovedCluster: {
                    this->handleNodeRemovedClusterEvent(event);
                }
                    break;
            }
        }

        void
        ServerRelayModule::handleServerMessage(const ServerMessage &message, const std::string connectionIdentifier) {
            LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 正在处理" << message.messagetype());

            auto userStateManagerService = this->mUserStateManagerServicePtr.lock();
            auto serverManageService = this->mServerManageServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();

            if (!userStateManagerService || !serverManageService || !connectionOperationService) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " 无法处理" << message.messagetype()
                                                         << " 由于缺少userStateManagerService、serverManageService或者 connectionOperationService");
            }

            ServerMessage serverMessage(message);

            if (message.GetTypeName() == SessionMessage::default_instance().GetTypeName()) {
                SessionMessage sessionMessage;
                sessionMessage.ParseFromString(message.content());
                //1.根据targetServerID确定服务器
                try {
                    auto node = serverManageService->getNode(sessionMessage.targetserverid());
                    serverMessage.set_serverid(sessionMessage.targetserverid());
                    connectionOperationService->sendMessageThroughConnection(node.getServerConnectionIdentifier(),
                                                                             serverMessage);
                } catch (ServerManageService::NodeNotExitException &exception) {
                    LOG4CXX_DEBUG(this->logger,
                                  __FUNCTION__ << " serverID=" << sessionMessage.targetserverid() << "不存在");
                }
            } else {

                //1.根据targetUser确定所在的服务器
                const std::string targetUser = message.targetuser();

                auto loginList = userStateManagerService->queryUserLoginServer(targetUser);
                if (loginList.size()) {//用户已经登陆了某台节点
                    for (auto loginNode: loginList) {
                        if (loginNode == message.serverid()) {
                            continue;
                        }
                        try {
                            auto node = serverManageService->getNode(loginNode);
                            serverMessage.set_serverid(loginNode);
                            connectionOperationService->sendMessageThroughConnection(
                                    node.getServerConnectionIdentifier(), serverMessage);
                        } catch (ServerManageService::NodeNotExitException &exception) {
                            LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " serverID=" << loginNode << "不存在");
                        }
                    }
                } else {
                    LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 用户:" << targetUser << "当前并未登陆任何节点");
                }
            }
        }

        void ServerRelayModule::handleNewNodeJoinedClusterEvent(const ClusterEvent &event) {

        }

        void ServerRelayModule::handleNodeRemovedClusterEvent(const ClusterEvent &event) {

        }

        void ServerRelayModule::addEvent(ClusterEvent event) {
            this->mEventQueue.push(event);
        }
    }
}

