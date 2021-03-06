//
// Created by taroyuyu on 2018/1/7.
//

#include <zconf.h>
#include <memory>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include "OnlineStateModule.h"
#include "../../Common/EventBus/EventBus.h"
#include "../Events/UserLogoutEvent.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace node {
        OnlineStateModule::OnlineStateModule() : mEpollInstance(-1), messageEventfd(-1), eventQueuefd(-1),
                                                 userLogoutEventProto(new UserLogoutEvent("", "")),
                                                 nodeSecessionEventProto(new NodeSecessionEvent("")) {
            this->logger = log4cxx::Logger::getLogger(OnlineStateModuleLogger);
        }

        OnlineStateModule::~OnlineStateModule() {
            while (false == this->messageQueue.empty()) {
                this->messageQueue.pop();
            }
            while (!this->mEventQueue.empty()) {
                this->mEventQueue.pop();
            }
        }

        bool OnlineStateModule::init() {
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

        void OnlineStateModule::execute() {
            //向EventBus注册监听消息
            EventBus::getDefault().registerEvent(userLogoutEventProto->getEventType(), this);
            EventBus::getDefault().registerEvent(nodeSecessionEventProto->getEventType(), this);
            while (this->m_isStarted) {
                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为0.1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   1000);

                if (-1 == happedEventsCount) {
                    LOG4CXX_WARN(this->logger,
                                 typeid(this).name() << "" << __FUNCTION__ << " 等待Epil实例上的事件出错，errno =" << errno);
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

                                    if (event->getEventType() == this->userLogoutEventProto->getEventType()) {
                                        this->handleUserLogoutEvent(*static_cast<const UserLogoutEvent *>(event.get()));
                                    }else if(event->getEventType() == this->nodeSecessionEventProto->getEventType()){
                                        this->handleNodeSecessionEvent(*static_cast<const NodeSecessionEvent*>(event.get()));
                                    }
                                }
                            } else {
                                LOG4CXX_WARN(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                               << "read(messageEventfd)操作出错，errno ="
                                                                               << errno);
                            }
                        } else if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->messageQueue.empty()) {
                                    this->messageQueueMutex.lock();
                                    auto pairIt = std::move(this->messageQueue.front());
                                    this->messageQueue.pop();
                                    this->messageQueueMutex.unlock();

                                    auto messageType = pairIt.first->GetTypeName();
                                    if (messageType ==
                                        kakaIM::Node::OnlineStateMessage::default_instance().GetTypeName()) {
                                        handleOnlineMessage(*(kakaIM::Node::OnlineStateMessage *) pairIt.first.get(),
                                                            pairIt.second);
                                    } else if (messageType ==
                                               kakaIM::president::UserOnlineStateMessage::default_instance().GetTypeName()) {
                                        handleOnlineMessage(
                                                *(kakaIM::president::UserOnlineStateMessage *) pairIt.first.get());
                                    }
                                }

                            } else {
                                LOG4CXX_WARN(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                               << "read(messageEventfd)操作出错，errno ="
                                                                               << errno);
                            }
                        }
                    }
                }
            }
            //向EventBus取消注册
            EventBus::getDefault().unregister(this);
            //向ClusterService取消注册
            if (auto clusterServicePtr = this->mClusterServicePtr.lock()) {
                clusterServicePtr->removeUserOnlineStateListener(*this);
            }
        }

        void OnlineStateModule::addOnlineStateMessage(const kakaIM::Node::OnlineStateMessage &message,
                                                      const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::OnlineStateMessage> onlineStateMessage(
                    new kakaIM::Node::OnlineStateMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(
                    std::move(onlineStateMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));

        }

        void OnlineStateModule::didReceivedUserOnlineStateFromCluster(
                const kakaIM::president::UserOnlineStateMessage &userOnlineStateMessage) {
            std::unique_ptr<kakaIM::president::UserOnlineStateMessage> userOnlineStateMessageFromCluster(
                    new kakaIM::president::UserOnlineStateMessage(userOnlineStateMessage));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(userOnlineStateMessageFromCluster), "");
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void OnlineStateModule::handleOnlineMessage(const kakaIM::Node::OnlineStateMessage &onlineStateMessage,
                                                    const std::string connectionIdentifier) {
            std::string userAccount = onlineStateMessage.useraccount();
            std::string sessionID = onlineStateMessage.sessionid();
            enum kakaIM::Node::OnlineStateMessage_OnlineState userState = onlineStateMessage.userstate();
            LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 用户:" << userAccount << " sessionID=" << sessionID);

            //更加用户在线状态
            auto setPairIt = this->mUserStateDB.find(userAccount);
            if (setPairIt != this->mUserStateDB.end()) {
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << "用户:" << userAccount << " 的登陆集存在");
                auto loginItemIt = setPairIt->second.begin();
                for (; loginItemIt != setPairIt->second.end(); ++loginItemIt) {
                    if ((loginItemIt->first.first == IDType_SessionID) && (loginItemIt->first.second == sessionID)) {
                        if (userState == kakaIM::Node::OnlineStateMessage_OnlineState_Offline) {
                            setPairIt->second.erase(loginItemIt);
                        } else {
                            auto loginDevicePair = loginItemIt->first;
                            if (loginItemIt->second != userState) {
                                setPairIt->second.erase(loginItemIt);
                                setPairIt->second.emplace(loginDevicePair, userState);
                            }
                        }
                        break;
                    }
                }

                if (loginItemIt == setPairIt->second.end() &&
                    userState != kakaIM::Node::OnlineStateMessage_OnlineState_Offline) {
                    setPairIt->second.emplace(std::make_pair(IDType_SessionID, sessionID), userState);
                }

                this->updateUserOnlineStateInThisNode(userAccount);

            } else {
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << "用户:" << userAccount << " 的登陆集不存在");
                if (userState != Node::OnlineStateMessage_OnlineState_Offline) {
                    LOG4CXX_DEBUG(this->logger, __FUNCTION__ << "为用户:" << userAccount << " 创建登陆集");
                    std::set<std::pair<std::pair<IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState >> loginSet;
                    loginSet.emplace(std::make_pair(IDType_SessionID, sessionID), userState);
                    LOG4CXX_DEBUG(this->logger, __FUNCTION__ << "将用户:" << userAccount << "的登陆设备加入登陆集");
                    this->mUserStateDB.emplace(userAccount, loginSet);
                    //向集群更新此用户的在线状态
                    if (auto clusterServicePtr = this->mClusterServicePtr.lock()) {
                        clusterServicePtr->updateUserOnlineState(onlineStateMessage);
                    }
                }
            }
        }

        void OnlineStateModule::updateUserOnlineStateInThisNode(const std::string userAccount) {
            if (auto clusterServicePtr = this->mClusterServicePtr.lock()) {

                auto setPairIt = this->mUserStateDB.find(userAccount);
                if (setPairIt == this->mUserStateDB.end()) {
                    return;;
                }

                auto loginSet = setPairIt->second;

                LOG4CXX_DEBUG(this->logger,__FUNCTION__<<"userAccount="<<userAccount);
                //2.判断此Node上，此userAccount的状态信息
                size_t userOnNodeCount = 0;
                Node::OnlineStateMessage_OnlineState userNodeState = Node::OnlineStateMessage_OnlineState_Offline;
                for (auto loginItemIt = loginSet.begin(); loginItemIt != loginSet.end(); ++loginItemIt) {
                    if (IDType_SessionID == loginItemIt->first.first) {
                        ++userOnNodeCount;
                        switch (loginItemIt->second) {
                            case Node::OnlineStateMessage_OnlineState_Online: {
                                userNodeState = Node::OnlineStateMessage_OnlineState_Online;
                                break;
                            }
                            case Node::OnlineStateMessage_OnlineState_Invisible: {
                                userNodeState = Node::OnlineStateMessage_OnlineState_Invisible;
                                continue;
                            }
                            case Node::OnlineStateMessage_OnlineState_Offline: {
                                --userOnNodeCount;
                                continue;
                            }
                        }
                    }
                }

                LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" userAccount="<<userAccount<<" userOnNodeCount="<<userOnNodeCount);
                if (0 == userOnNodeCount) {//向集群更新：以userAccount登陆此Node的所有设备全部下线
                    LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" 向集群更新:以"<<userAccount<<"登陆此Node的所有设备全部下线");
                    kakaIM::Node::OnlineStateMessage offlineMessage;
                    offlineMessage.set_useraccount(userAccount);
                    offlineMessage.set_userstate(Node::OnlineStateMessage_OnlineState_Offline);
                    clusterServicePtr->updateUserOnlineState(offlineMessage);
                } else {//向集群更新:以userAccount登陆此Node的所有设备的最高状态
                    kakaIM::Node::OnlineStateMessage userStateMessage;
                    userStateMessage.set_useraccount(userAccount);
                    userStateMessage.set_userstate(userNodeState);
                    clusterServicePtr->updateUserOnlineState(userStateMessage);
                }
            }
        }

        void
        OnlineStateModule::handleOnlineMessage(const kakaIM::president::UserOnlineStateMessage &onlineStateMessage) {
            std::string userAccount = onlineStateMessage.useraccount();
            std::string serverID = onlineStateMessage.serverid();
            enum Node::OnlineStateMessage_OnlineState userState = Node::OnlineStateMessage_OnlineState_Offline;
            switch (onlineStateMessage.userstate()) {
                case president::UserOnlineStateMessage_OnlineState_Online: {
                    userState = Node::OnlineStateMessage_OnlineState_Online;
                }
                    break;
                case president::UserOnlineStateMessage_OnlineState_Invisible: {
                    userState = Node::OnlineStateMessage_OnlineState_Invisible;
                }
                    break;
                case president::UserOnlineStateMessage_OnlineState_Offline: {
                    userState = Node::OnlineStateMessage_OnlineState_Offline;
                }
                    break;
                default:
                    break;
            }

            //更新用户状态
            auto setPairIt = this->mUserStateDB.find(userAccount);
            if (setPairIt != this->mUserStateDB.end()) {
                auto loginItemIt = setPairIt->second.begin();
                for (; loginItemIt != setPairIt->second.end(); ++loginItemIt) {
                    if ((loginItemIt->first.first == IDType_ServerID) && (loginItemIt->first.second == serverID)) {
                        if (userState == kakaIM::Node::OnlineStateMessage_OnlineState_Offline) {
                            setPairIt->second.erase(loginItemIt);
                        } else {
                            auto loginDevicePair = loginItemIt->first;
                            if (loginItemIt->second != userState) {
                                setPairIt->second.erase(loginItemIt);
                                setPairIt->second.emplace(loginDevicePair, userState);
                            }
                        }
                        break;
                    }
                }
                if (loginItemIt == setPairIt->second.end()) {
                    setPairIt->second.emplace(std::make_pair(IDType_ServerID, serverID), userState);
                }
            } else {
                std::set<std::pair<std::pair<IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState >> loginSet;
                loginSet.emplace(std::make_pair(IDType_ServerID, serverID), userState);
                this->mUserStateDB.emplace(userAccount, loginSet);
            }
        }

        kakaIM::Node::OnlineStateMessage_OnlineState OnlineStateModule::getUserOnlineState(std::string userAccount) {
            auto setPairIt = this->mUserStateDB.find(userAccount);
            if (setPairIt != this->mUserStateDB.end()) {
                auto loginSet = setPairIt->second;
                bool OnlineStateMessage_OnlineState_Online_flag = false;
                bool OnlineStateMessage_OnlineState_Invisible_flag = false;
                for (auto loginItemIt = loginSet.begin(); loginItemIt != loginSet.end(); ++loginItemIt) {

                    if (kakaIM::Node::OnlineStateMessage_OnlineState_Online == loginItemIt->second) {
                        OnlineStateMessage_OnlineState_Online_flag = true;
                        break;
                    } else if (kakaIM::Node::OnlineStateMessage_OnlineState_Invisible == loginItemIt->second) {
                        OnlineStateMessage_OnlineState_Invisible_flag = true;
                        break;
                    }
                }

                if (OnlineStateMessage_OnlineState_Online_flag) {
                    return kakaIM::Node::OnlineStateMessage_OnlineState_Online;
                } else if (OnlineStateMessage_OnlineState_Invisible_flag) {
                    return kakaIM::Node::OnlineStateMessage_OnlineState_Invisible;
                } else {
                    return kakaIM::Node::OnlineStateMessage_OnlineState_Online;
                }
            } else {
                return kakaIM::Node::OnlineStateMessage_OnlineState_Offline;
            }
        }

        void OnlineStateModule::onEvent(std::shared_ptr<const Event> event) {
            LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" 被调用");
            std::lock_guard<std::mutex> lock(this->eventQueueMutex);
            this->mEventQueue.emplace(event);
            uint64_t count = 1;
            //增加信号量
            ::write(this->eventQueuefd, &count, sizeof(count));
        }

        void OnlineStateModule::handleUserLogoutEvent(const UserLogoutEvent &event) {
            if (std::shared_ptr<const QueryUserAccountWithSession> service = this->mQueryUserAccountWithSessionServicePtr.lock()) {
                std::string sessionID = event.getSessionID();
                std::string userAccount = event.getUserAccount();
                auto setPairIt = this->mUserStateDB.find(userAccount);
                if (setPairIt != this->mUserStateDB.end()) {
                    for (auto loginItemIt = setPairIt->second.begin();
                         loginItemIt != setPairIt->second.end(); ++loginItemIt) {
                        if (loginItemIt->first.first == IDType_SessionID &&
                            loginItemIt->first.second == sessionID) {
                            setPairIt->second.erase(loginItemIt);
                            LOG4CXX_DEBUG(this->logger,__FUNCTION__<<" 移除完毕");
                            break;
                        }
                    }
                    this->updateUserOnlineStateInThisNode(userAccount);
                }
            }
        }

        void OnlineStateModule::handleNodeSecessionEvent(const NodeSecessionEvent & event){
            LOG4CXX_DEBUG(this->logger,__FUNCTION__);
            const std::string serverID = event.getServerID();
            for(auto setIt = this->mUserStateDB.begin(); setIt != this->mUserStateDB.end();++setIt){
                auto recordIt = std::find_if(setIt->second.begin(),setIt->second.end(),[serverID](const std::set<std::pair<std::pair<IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::value_type & item)-> bool{
                    return item.first.first == IDType_ServerID && item.first.second == serverID;
                });
                if(recordIt != setIt->second.end()){
                    setIt->second.erase(recordIt);
                }
            }
        }

        std::pair<std::set<std::pair<std::pair<LoginDeviceQueryService::IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator, std::set<std::pair<std::pair<LoginDeviceQueryService::IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator>
        OnlineStateModule::queryLoginDeviceSetWithUserAccount(const std::string &userAccount) const {
            auto setPairIt = this->mUserStateDB.find(userAccount);
            if (setPairIt != this->mUserStateDB.end()) {
                LOG4CXX_DEBUG(this->logger,
                              __FUNCTION__ << " 用户:" << userAccount << " 当前在线设备数量:" << setPairIt->second.size());
                return std::make_pair(setPairIt->second.begin(), setPairIt->second.end());
            } else {
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 用户:" << userAccount << " 当前在线设备数量:0");
                std::set<std::pair<std::pair<LoginDeviceQueryService::IDType, std::string>, kakaIM::Node::OnlineStateMessage_OnlineState>>::const_iterator endIt;
                return std::make_pair(endIt, endIt);
            }
        }

        void OnlineStateModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void OnlineStateModule::setClusterService(std::weak_ptr<ClusterService> clusterServicePtr) {
            if (auto clusterServicePtr = this->mClusterServicePtr.lock()) {
                clusterServicePtr->removeUserOnlineStateListener(*this);
            }
            this->mClusterServicePtr = clusterServicePtr;
            if (auto clusterServicePtr = this->mClusterServicePtr.lock()) {
                clusterServicePtr->addUserOnlineStateListener(*this);
            }
        }

        void OnlineStateModule::setQueryUserAccountWithSessionService(
                std::weak_ptr<QueryUserAccountWithSession> queryUserAccountWithSessionServicePtr) {
            this->mQueryUserAccountWithSessionServicePtr = queryUserAccountWithSessionServicePtr;
        }
    }
}
