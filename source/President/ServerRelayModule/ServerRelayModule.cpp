//
// Created by Kakawater on 2018/1/11.
//

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <zconf.h>
#include <typeinfo>
#include "ServerRelayModule.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace president {
        bool ServerRelayModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }
            this->eventQueuefd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->eventQueuefd < 0) {
                return false;
            }
            //创建Epoll实例
            if (-1 == (this->mEpollInstance = epoll_create1(0))) {
                return false;
            }
            //向Epill实例注册messageEventfd,和clusterEventfd
            struct epoll_event messageEventfdEvent;
            messageEventfdEvent.events = EPOLLIN;
            messageEventfdEvent.data.fd = this->messageEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->messageEventfd, &messageEventfdEvent)) {
                return false;
            }
            struct epoll_event eventQueuefdEvent;
            eventQueuefdEvent.events = EPOLLIN;
            eventQueuefdEvent.data.fd = this->eventQueuefd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->eventQueuefd, &eventQueuefdEvent)) {
                return false;
            }
            return true;
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

        void ServerRelayModule::shouldStop() {
            this->m_needStop = true;
        }

        void ServerRelayModule::dispatchMessage(std::pair<std::unique_ptr<ServerMessage>, const std::string> & task){
            this->handleServerMessage(*(task.first.get()), task.second);
        }

        void ServerRelayModule::dispatchClusterEvent(ClusterEvent & event){
            switch (event.getEventType()) {
                case ClusterEvent::NewNodeJoinedCluster: {
                    this->handleNewNodeJoinedClusterEvent(event);
                }
                    break;
                case ClusterEvent::NodeRemovedCluster: {
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

        void ServerRelayModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void ServerRelayModule::setUserStateManagerService(
                std::weak_ptr<UserStateManagerService> userStateManagerServicePtr) {
            this->mUserStateManagerServicePtr = userStateManagerServicePtr;
        }

        void ServerRelayModule::setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr) {
            this->mServerManageServicePtr = serverManageServicePtr;
        }

        void ServerRelayModule::addServerMessage(std::unique_ptr<ServerMessage> message,
                                                 const std::string connectionIdentifier) {
            if (!message) {
                return;
            }
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(message), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));

        }

        void ServerRelayModule::addEvent(ClusterEvent event) {
            std::lock_guard<std::mutex> lock(this->eventQueueMutex);
            this->mEventQueue.emplace(event);
            uint64_t count = 1;
            //增加信号量
            ::write(this->eventQueuefd, &count, sizeof(count));
        }

        ServerRelayModule::ServerRelayModule() : mEpollInstance(-1), messageEventfd(-1), eventQueuefd(-1) {
            this->logger = log4cxx::Logger::getLogger(ServerRelayModuleLogger);
        }

        ServerRelayModule::~ServerRelayModule() {
            while (false == this->messageQueue.empty()) {
                this->messageQueue.pop();
            }
        }


    }
}

