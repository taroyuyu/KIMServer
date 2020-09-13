//
// Created by taroyuyu on 2018/1/11.
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
            while (this->m_isStarted) {
                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为0.1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   1000);

                if (-1 == happedEventsCount) {
		    LOG4CXX_WARN(this->logger, typeid(this).name()<<""<<__FUNCTION__<<" 等待Epoll实例上的事件出错，errno ="<<errno);
                }

                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (EPOLLIN & happedEvents[i].events) {
                        if (this->eventQueuefd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < read(this->eventQueuefd, &count, sizeof(count))) {
                                while (count-- && false == this->mEventQueue.empty()) {
                                    this->eventQueueMutex.lock();
                                    auto event = std::move(this->mEventQueue.front());
                                    this->mEventQueue.pop();
                                    this->eventQueueMutex.unlock();

                                    switch (event.getEventType()){
                                        case ClusterEvent::NewNodeJoinedCluster:{
                                            this->handleNewNodeJoinedClusterEvent(event);
                                        }
                                            break;
                                        case ClusterEvent::NodeRemovedCluster:{
                                            this->handleNodeRemovedClusterEvent(event);
                                        }
                                            break;
                                        default:{

                                        }
                                    }
                                }
                            } else {
				LOG4CXX_WARN(this->logger, typeid(this).name()<<""<<__FUNCTION__<<"read(eventqueuefd)操作出错，errno ="<<errno);
                            }
                        } else if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < ::read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->messageQueue.empty()) {
                                    this->messageQueueMutex.lock();
                                    auto pairIt = std::move(this->messageQueue.front());
                                    this->messageQueue.pop();
                                    this->messageQueueMutex.unlock();

                                    this->handleServerMessage(*(pairIt.first.get()), pairIt.second);
                                }
                            } else {
				LOG4CXX_WARN(this->logger, typeid(this).name()<<""<<__FUNCTION__<<"read(messageEventfd)操作出错，errno ="<<errno);
                            }
                        }
                    }
                }
            }
        }

        void ServerRelayModule::handleServerMessage(const ServerMessage &message, const std::string connectionIdentifier) {
            LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" 正在处理"<<message.messagetype());
            //1.根据targetUser确定所在的服务器
            const std::string targetUser = message.targetuser();
            auto userStateManagerService = this->mUserStateManagerServicePtr.lock();
            auto serverManageService = this->mServerManageServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();

            if(!userStateManagerService || !serverManageService || !connectionOperationService){
                LOG4CXX_ERROR(this->logger,__FUNCTION__<<" 无法处理"<<message.messagetype()<<" 由于缺少userStateManagerService、serverManageService或者 connectionOperationService");
            }
            auto loginList = userStateManagerService->queryUserLoginServer(targetUser);
            ServerMessage serverMessage(message);
            if(loginList.size()){//用户已经登陆了某台节点
                for(auto loginNode : loginList){
                    if(loginNode == message.serverid()){
                        continue;
                    }
                    try {
                        auto node = serverManageService->getNode(loginNode);
                        serverMessage.set_serverid(loginNode);
                        connectionOperationService->sendMessageThroughConnection(node.getServerConnectionIdentifier(),serverMessage);
                    }catch (ServerManageService::NodeNotExitException & exception){
                        LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" serverID="<<loginNode<<"不存在");
                    }
                }
            }else{
                LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" 用户:"<<targetUser<<"当前并未登陆任何节点");
            }
        }

        void ServerRelayModule::handleNewNodeJoinedClusterEvent(const ClusterEvent &event) {

        }

        void ServerRelayModule::handleNodeRemovedClusterEvent(const ClusterEvent &event) {

        }

        void ServerRelayModule::setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr){
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void ServerRelayModule::setUserStateManagerService(std::weak_ptr<UserStateManagerService> userStateManagerServicePtr){
            this->mUserStateManagerServicePtr = userStateManagerServicePtr;
        }

        void ServerRelayModule::setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr){
            this->mServerManageServicePtr = serverManageServicePtr;
        }

        void ServerRelayModule::addServerMessage(const ServerMessage &message, const std::string connectionIdentifier) {
            std::unique_ptr<ServerMessage> serverMessage(new ServerMessage(message));
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(serverMessage), connectionIdentifier);
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

        ServerRelayModule::ServerRelayModule() : mEpollInstance(-1),messageEventfd(-1),eventQueuefd(-1) {
	    this->logger = log4cxx::Logger::getLogger(ServerRelayModuleLogger);
        }

        ServerRelayModule::~ServerRelayModule() {
            while (false == this->messageQueue.empty()) {
                this->messageQueue.pop();
            }
        }


    }
}

