//
// Created by Kakawater on 2018/1/1.
//


#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <typeinfo>
#include "UserStateManagerModule.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace president {
        bool UserStateManagerModule::init() {
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }
            this->clusterEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->clusterEventfd < 0) {
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
            struct epoll_event clusterEventfdEvent;
            clusterEventfdEvent.events = EPOLLIN;
            clusterEventfdEvent.data.fd = this->clusterEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->clusterEventfd, &clusterEventfdEvent)) {
                return false;
            }
            return true;
        }

        void UserStateManagerModule::execute() {
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }
            while (not this->m_needStop) {
                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   10000);

                if (-1 == happedEventsCount) {
                    LOG4CXX_WARN(this->logger,
                                 typeid(this).name() << "" << __FUNCTION__ << " 等待Epoll实例上的事件出错，errno =" << errno);
                }

                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (EPOLLIN & happedEvents[i].events) {
                        if (this->clusterEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < read(this->clusterEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->mEventQueue.empty()) {
                                    this->eventQueueMutex.lock();
                                    auto event = std::move(this->mEventQueue.front());
                                    this->mEventQueue.pop();
                                    this->eventQueueMutex.unlock();

                                    switch (event.getEventType()) {
                                        case ClusterEvent::NewNodeJoinedCluster: {
                                            this->handleNewNodeJoinedClusterEvent(event);
                                        }
                                            break;
                                        case ClusterEvent::NodeRemovedCluster: {
                                            this->handleNodeRemovedClusterEvent(event);
                                        }
                                            break;
                                        default:
                                            break;
                                    }
                                }
                            }
                        } else if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->messageQueue.empty()) {
                                    this->messageQueueMutex.lock();
                                    auto pairIt = std::move(this->messageQueue.front());
                                    this->messageQueue.pop();
                                    this->messageQueueMutex.unlock();

                                    std::string messageType = pairIt.first->GetTypeName();
                                    if (messageType == UpdateUserOnlineStateMessage::default_instance().GetTypeName()) {
                                        this->handleUpdateUserOnlineStateMessage(
                                                *(const UpdateUserOnlineStateMessage *) pairIt.first.get(),
                                                pairIt.second);
                                    } else if (messageType ==
                                               UserOnlineStateMessage::default_instance().GetTypeName()) {
                                        this->handleUserOnlineStateMessage(
                                                *(const UserOnlineStateMessage *) pairIt.first.get(), pairIt.second);
                                    } else {

                                    }
                                }
                            } else {
                                LOG4CXX_WARN(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                               << "read(messageEventfd)操作出错，errno ="
                                                                               << errno);
                            }
                        }
                    } else {

                    }
                }
            }

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }

        void UserStateManagerModule::shouldStop(){
            this->m_needStop = true;
        }

        void UserStateManagerModule::handleUpdateUserOnlineStateMessage(const UpdateUserOnlineStateMessage &message,
                                                                        const std::string connectionIdentifier) {
            //for (int i = 0; i < message->itemcount(); ++i) {
            //  UserOnlineStateMessage userOnlineState = message->useronlinestate(i);
//                auto it = this->userOnlineState.find(userOnlineState.useraccount());
//                if (it == this->userOnlineState.end()){
//                    std::vector onlineList;
//                    UserOnlineStateMessage_OnlineState
//                    userOnlineState.userstate()
//                    onlineList.push_back(std::make_pair(userOnlineState.serverid(),userOnlineState.userstate()));
//                    this->userOnlineState[userOnlineState.useraccount()] = onlineList;
//                }else{
////                    it->second.push_back(std::make_pair(userOnlineState.serverid(),userOnlineState.userstate()));
//
//                    if
//                }
            //    }
        }

        void UserStateManagerModule::handleUserOnlineStateMessage(const UserOnlineStateMessage &userOnlineStateMessage,
                                                                  const std::string connectionIdentifier) {

            LOG4CXX_DEBUG(this->logger, __FUNCTION__);
            //1.提取信息
            std::string userAccount = userOnlineStateMessage.useraccount();
            std::string serverID = userOnlineStateMessage.serverid();
            UserOnlineStateMessage_OnlineState userState = userOnlineStateMessage.userstate();
            //2.更新
            this->updateUserOnlineState(userAccount, serverID, userState);
            //3.推送到其余Server
            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                if (auto serverManageServicePtr = this->mServerManageServicePtr.lock()) {
                    std::set<Node> nodeSet = serverManageServicePtr->getAllNodes();
                    for (auto nodeIt = nodeSet.begin(); nodeIt != nodeSet.end(); ++nodeIt) {
                        if (nodeIt->getServerID() != serverID) {
                            LOG4CXX_DEBUG(this->logger, __FUNCTION__ << "推送给其余节点");
                            connectionOperationService->sendMessageThroughConnection(
                                    nodeIt->getServerConnectionIdentifier(), userOnlineStateMessage);
                        }
                    }
                }
            } else {
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " 无法将用户在线状态推送到Server,由于缺少connectionOperationService");
            }
        }

        void UserStateManagerModule::handleNewNodeJoinedClusterEvent(const ClusterEvent &event) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
            //向新节点推送当前集群的用户在线状态
            UpdateUserOnlineStateMessage updateUserOnlineStateMessage;
            for(auto loginSetIt = this->mUserOnlineStateDB.begin(); loginSetIt != this->mUserOnlineStateDB.end();++loginSetIt){
                for (auto loginNodeIt = loginSetIt->second.begin(); loginNodeIt != loginSetIt->second.end();++loginNodeIt){
                    auto userOnlineState = updateUserOnlineStateMessage.add_useronlinestate();
                    userOnlineState->set_useraccount(loginSetIt->first);
                    userOnlineState->set_serverid(loginNodeIt->first);
                    userOnlineState->set_userstate(loginNodeIt->second);
                }
            }
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto serverManagerService = this->mServerManageServicePtr.lock();
            if(!serverManagerService || !connectionOperationService){
                LOG4CXX_ERROR(this->logger,__FUNCTION__<<" 无法向新节点:"<<event.getTargetServerID()<<" 推送用户在线状态,由于缺少connectionOperationService或serverManagerService");
                return;
            }

            try {
                auto node = serverManagerService->getNode(event.getTargetServerID());
                connectionOperationService->sendMessageThroughConnection(node.getServerConnectionIdentifier(),updateUserOnlineStateMessage);
            }catch (ServerManageService::NodeNotExitException & exception){

            }

        }

        void UserStateManagerModule::handleNodeRemovedClusterEvent(const ClusterEvent &event) {
            //移除此节点上的所有在线用户
            this->removeServer(event.getTargetServerID());
        }

        void UserStateManagerModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void UserStateManagerModule::addUpdateUserOnlineStateMessage(std::unique_ptr<UpdateUserOnlineStateMessage>message,
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

        void UserStateManagerModule::addUserOnlineStateMessage(std::unique_ptr<UserOnlineStateMessage> message,
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

        void UserStateManagerModule::addEvent(ClusterEvent event) {
            std::lock_guard<std::mutex> lock(this->eventQueueMutex);
            this->mEventQueue.emplace(event);
            uint64_t count = 1;
            //增加信号量
            ::write(this->clusterEventfd, &count, sizeof(count));
        }

        void UserStateManagerModule::setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr) {
            this->mServerManageServicePtr = serverManageServicePtr;
        }

        std::list<std::string> UserStateManagerModule::queryUserLoginServer(std::string userAccount) {
            std::list<std::string> loginServerList;
            auto userLoginSetIt = this->mUserOnlineStateDB.find(userAccount);
            if(userLoginSetIt != this->mUserOnlineStateDB.end()){
                for (auto itemIt = userLoginSetIt->second.begin();itemIt != userLoginSetIt->second.end(); ++itemIt) {
                    loginServerList.emplace_back(itemIt->first);
                }
            }
            return loginServerList;
        }

        void UserStateManagerModule::removeServer(const std::string serverID){
            for(auto loginSetIt = this->mUserOnlineStateDB.begin();loginSetIt != this->mUserOnlineStateDB.end();++loginSetIt){
                auto recordIt = std::find_if(loginSetIt->second.begin(),loginSetIt->second.end(),[serverID](const std::set<std::pair<std::string,UserOnlineStateMessage_OnlineState>>::value_type & item)-> bool{
                    return item.first == serverID;
                });

                if(recordIt != loginSetIt->second.end()){
                    loginSetIt->second.erase(recordIt);
                }
            }
        }

        void UserStateManagerModule::updateUserOnlineState(std::string userAccount,std::string serverID,UserOnlineStateMessage_OnlineState onlineState){
            auto userLoginSetIt = this->mUserOnlineStateDB.find(userAccount);
            if(userLoginSetIt != this->mUserOnlineStateDB.end()){
                auto itemIt = std::find_if(userLoginSetIt->second.begin(),userLoginSetIt->second.end(),[userAccount](const std::set<std::pair<std::string,UserOnlineStateMessage_OnlineState>>::value_type & item)->bool {
                    if(item.first == userAccount){
                        return true;
                    }else{
                        return false;
                    }
                });

                if(itemIt != userLoginSetIt->second.end()){

                    if(UserOnlineStateMessage_OnlineState_Offline == onlineState){//userAccount在serverID上的所有登陆设备全部下线
                        userLoginSetIt->second.erase(itemIt);
                    }else{
                        //更新userAccount在serverID上的所有登陆设备的状态
                        userLoginSetIt->second.erase(itemIt);
                        userLoginSetIt->second.emplace(serverID,onlineState);
                    }
                }else{
                    userLoginSetIt->second.emplace(serverID,onlineState);
                }
            }else{
                std::set<std::pair<std::string,UserOnlineStateMessage_OnlineState>> userLoginSet;
                userLoginSet.emplace(serverID,onlineState);
                this->mUserOnlineStateDB.emplace(userAccount,userLoginSet);

            }
        }

        UserStateManagerModule::UserStateManagerModule() : mEpollInstance(-1), messageEventfd(-1), clusterEventfd(-1) {
            this->logger = log4cxx::Logger::getLogger(UserStateManagerModuleLogger);
        }

        UserStateManagerModule::~UserStateManagerModule() {
            while (false == this->messageQueue.empty()) {
                this->messageQueue.pop();
            }
        }
    }
}

