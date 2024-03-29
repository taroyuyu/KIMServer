//
// Created by Kakawater on 2018/1/1.
//

#include <typeinfo>
#include <President/UserStateManagerModule/UserStateManagerModule.h>
#include <President/Log/log.h>

namespace kakaIM {
    namespace president {
        UserStateManagerModule::UserStateManagerModule() : KIMPresidentModule(UserStateManagerModuleLogger) {
            this->mMessageHandlerSet[UpdateUserOnlineStateMessage::default_instance().GetTypeName()] = [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier){
                this->handleUpdateUserOnlineStateMessage(*(const UpdateUserOnlineStateMessage *) message.get(),connectionIdentifier);
            };
            this->mMessageHandlerSet[UserOnlineStateMessage::default_instance().GetTypeName()] = [this](std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier){
                this->handleUserOnlineStateMessage(*(const UserOnlineStateMessage *) message.get(), connectionIdentifier);
            };
        }

        UserStateManagerModule::~UserStateManagerModule() {
        }
        const std::unordered_set<std::string> UserStateManagerModule::messageTypes(){
            std::unordered_set<std::string> messageTypeSet;
            for(auto & record : this->mMessageHandlerSet){
                messageTypeSet.insert(record.first);
            }
            return messageTypeSet;
        }
        void UserStateManagerModule::execute() {
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

                if (auto event = this->mEventQueue.try_pop()) {
                    this->dispatchClusterEvent(*event);
                    needSleep = false;
                }

                if (needSleep) {
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

        void UserStateManagerModule::dispatchMessage(
                std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> &task) {
            const auto messageType = task.first->GetTypeName();
            auto it = this->mMessageHandlerSet.find(messageType);
            if (it != this->mMessageHandlerSet.end()){
                it->second(std::move(task.first),task.second);
            }
        }

        void UserStateManagerModule::dispatchClusterEvent(ClusterEvent &event) {
            switch (event.getEventType()) {
                case ClusterEvent::ClusterEventType::NewNodeJoinedCluster: {
                    this->handleNewNodeJoinedClusterEvent(event);
                }
                    break;
                case ClusterEvent::ClusterEventType::NodeRemovedCluster: {
                    this->handleNodeRemovedClusterEvent(event);
                }
                    break;
                default:
                    break;
            }
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
                LOG4CXX_ERROR(this->logger,
                              __FUNCTION__ << " 无法将用户在线状态推送到Server,由于缺少connectionOperationService");
            }
        }

        void UserStateManagerModule::handleNewNodeJoinedClusterEvent(const ClusterEvent &event) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            //向新节点推送当前集群的用户在线状态
            UpdateUserOnlineStateMessage updateUserOnlineStateMessage;
            for (auto loginSetIt = this->mUserOnlineStateDB.begin();
                 loginSetIt != this->mUserOnlineStateDB.end(); ++loginSetIt) {
                for (auto loginNodeIt = loginSetIt->second.begin();
                     loginNodeIt != loginSetIt->second.end(); ++loginNodeIt) {
                    auto userOnlineState = updateUserOnlineStateMessage.add_useronlinestate();
                    userOnlineState->set_useraccount(loginSetIt->first);
                    userOnlineState->set_serverid(loginNodeIt->first);
                    userOnlineState->set_userstate(loginNodeIt->second);
                }
            }
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto serverManagerService = this->mServerManageServicePtr.lock();
            if (!serverManagerService || !connectionOperationService) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " 无法向新节点:" << event.getTargetServerID()
                                                         << " 推送用户在线状态,由于缺少connectionOperationService或serverManagerService");
                return;
            }

            try {
                auto node = serverManagerService->getNode(event.getTargetServerID());
                connectionOperationService->sendMessageThroughConnection(node.getServerConnectionIdentifier(),
                                                                         updateUserOnlineStateMessage);
            } catch (ServerManageService::NodeNotExitException &exception) {

            }

        }

        void UserStateManagerModule::handleNodeRemovedClusterEvent(const ClusterEvent &event) {
            //移除此节点上的所有在线用户
            this->removeServer(event.getTargetServerID());
        }

        void UserStateManagerModule::addEvent(ClusterEvent event) {
            this->mEventQueue.push(event);
        }

        std::list<std::string> UserStateManagerModule::queryUserLoginServer(std::string userAccount) {
            std::list<std::string> loginServerList;
            auto userLoginSetIt = this->mUserOnlineStateDB.find(userAccount);
            if (userLoginSetIt != this->mUserOnlineStateDB.end()) {
                for (auto itemIt = userLoginSetIt->second.begin(); itemIt != userLoginSetIt->second.end(); ++itemIt) {
                    loginServerList.emplace_back(itemIt->first);
                }
            }
            return loginServerList;
        }

        void UserStateManagerModule::removeServer(const std::string serverID) {
            for (auto loginSetIt = this->mUserOnlineStateDB.begin();
                 loginSetIt != this->mUserOnlineStateDB.end(); ++loginSetIt) {
                auto recordIt = std::find_if(loginSetIt->second.begin(), loginSetIt->second.end(), [serverID](
                        const std::set<std::pair<std::string, UserOnlineStateMessage_OnlineState>>::value_type &item) -> bool {
                    return item.first == serverID;
                });

                if (recordIt != loginSetIt->second.end()) {
                    loginSetIt->second.erase(recordIt);
                }
            }
        }

        void UserStateManagerModule::updateUserOnlineState(std::string userAccount, std::string serverID,
                                                           UserOnlineStateMessage_OnlineState onlineState) {
            auto userLoginSetIt = this->mUserOnlineStateDB.find(userAccount);
            if (userLoginSetIt != this->mUserOnlineStateDB.end()) {
                auto itemIt = std::find_if(userLoginSetIt->second.begin(), userLoginSetIt->second.end(), [userAccount](
                        const std::set<std::pair<std::string, UserOnlineStateMessage_OnlineState>>::value_type &item) -> bool {
                    if (item.first == userAccount) {
                        return true;
                    } else {
                        return false;
                    }
                });

                if (itemIt != userLoginSetIt->second.end()) {

                    if (UserOnlineStateMessage_OnlineState_Offline == onlineState) {//userAccount在serverID上的所有登陆设备全部下线
                        userLoginSetIt->second.erase(itemIt);
                    } else {
                        //更新userAccount在serverID上的所有登陆设备的状态
                        userLoginSetIt->second.erase(itemIt);
                        userLoginSetIt->second.emplace(serverID, onlineState);
                    }
                } else {
                    userLoginSetIt->second.emplace(serverID, onlineState);
                }
            } else {
                std::set<std::pair<std::string, UserOnlineStateMessage_OnlineState>> userLoginSet;
                userLoginSet.emplace(serverID, onlineState);
                this->mUserOnlineStateDB.emplace(userAccount, userLoginSet);

            }
        }
    }
}

