//
// Created by taroyuyu on 2018/1/7.
//

#include <zconf.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "SingleChatModule.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace node {
        SingleChatModule::SingleChatModule() : mEpollInstance(-1),messageEventfd(-1) {
            this->logger = log4cxx::Logger::getLogger(SingleChatModuleLogger);
        }

        SingleChatModule::~SingleChatModule() {
        }

        void SingleChatModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void SingleChatModule::setMessageSendService(std::weak_ptr<MessageSendService> messageSendServicePtr) {
            this->mMessageSendServicePtr = messageSendServicePtr;
        }

        void SingleChatModule::setMessageIDGenerateService(std::weak_ptr<MessageIDGenerateService> service) {
            this->mMessageIDGenerateServicePtr = service;
        }

        void SingleChatModule::setClusterService(std::weak_ptr<ClusterService> service) {
            this->mClusterServicePtr = service;
        }

        void SingleChatModule::setLoginDeviceQueryService(std::weak_ptr<LoginDeviceQueryService> service) {
            this->mLoginDeviceQueryServicePtr = service;
        }

        void
        SingleChatModule::setQueryUserAccountWithSessionService(std::weak_ptr<QueryUserAccountWithSession> service) {
            this->mQueryUserAccountWithSessionServicePtr = service;
        }

        void SingleChatModule::setQueryConnectionWithSessionService(std::weak_ptr<QueryConnectionWithSession> service) {
            this->mQueryConnectionWithSessionServicePtr = service;
        }

        void SingleChatModule::setMessagePersistenceService(std::weak_ptr<MessagePersistenceService> service) {
            this->mMessagePersistenceServicePtr = service;
        }

        void SingleChatModule::setUserRelationService(std::weak_ptr<UserRelationService> service) {
            this->mUserRelationServicePtr = service;
        }

        bool SingleChatModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
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

            return true;
        }

        void SingleChatModule::execute() {
            while (this->m_isStarted) {
                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为0.1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   1000);

                if (-1 == happedEventsCount) {
                    LOG4CXX_WARN(this->logger,__FUNCTION__<<" 等待Epil实例上的事件出错，errno ="<<errno);
                }

                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (EPOLLIN & happedEvents[i].events) {
                        if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < ::read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->messageQueue.empty()) {
                                    this->messageQueueMutex.lock();
                                    auto pairIt = std::move(this->messageQueue.front());
                                    this->messageQueue.pop();
                                    this->messageQueueMutex.unlock();

                                    auto messageType = pairIt.first->GetTypeName();
                                    if (messageType == kakaIM::Node::ChatMessage::default_instance().GetTypeName()) {
                                        handleChatMessage(*(kakaIM::Node::ChatMessage *) pairIt.first.get(), pairIt.second);
                                    }
                                }

                            } else {
                                LOG4CXX_WARN(this->logger,
                                             typeid(this).name() << "" << __FUNCTION__ << "read(messageEventfd)操作出错，errno ="
                                                                 << errno);
                            }
                        }
                    }
                }
            }
        }

        void SingleChatModule::handleChatMessage(kakaIM::Node::ChatMessage &chatMessage,
                                                 const std::string connectionIdentifier) {
            const std::string senderAccount = chatMessage.senderaccount();
            const std::string receiverAccount = chatMessage.receiveraccount();
            const std::string senderDeviceSessionID = chatMessage.sessionid();
            auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock();
            auto messageIDGenerator = this->mMessageIDGenerateServicePtr.lock();
            auto messagePersister = this->mMessagePersistenceServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            auto userRelationService = this->mUserRelationServicePtr.lock();

            if (!(queryUserAccountService && messageIDGenerator && messagePersister && userRelationService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少mqueryUserAccountService、messageIDGenerator、userRelationService或者messagePersiste、messageSendService");
                return;
            }

            //1.验证消息发送者
            const std::string userAccount = queryUserAccountService->queryUserAccountWithSession(
                    chatMessage.sessionid());
            if (!userAccount.empty() && senderAccount != userAccount) {
                return;
            }

            if (senderAccount == receiverAccount) {
                return;
            }

            //2.验证消息发送者和消息接收者之间存在好友关系
            if (false == userRelationService->checkFriendRelation(userAccount, receiverAccount)) {//不存在好友关系
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 用户:" << senderAccount << " 和用户:" << receiverAccount
                                                         << "之间不存在好友关系,拒绝转发");
                return;
            }
            //3.将聊天消息保存到数据库
            //3.1获取各自的消息ID
            auto future = messageIDGenerator->generateMessageIDWithUserAccount(chatMessage.receiveraccount());
            uint64_t messageID_receiver = future->getMessageID();

            future = messageIDGenerator->generateMessageIDWithUserAccount(chatMessage.senderaccount());
            uint64_t messageID_sender = future->getMessageID();
            //3.2将聊天消息保存到数据库
            messagePersister->persistChatMessage(chatMessage.receiveraccount(), chatMessage, messageID_receiver);
            messagePersister->persistChatMessage(chatMessage.senderaccount(), chatMessage, messageID_sender);

            //4.将消息转发给接收方
            chatMessage.set_messageid(messageID_receiver);
            messageSendService->sendMessageToUser(receiverAccount, chatMessage);

            //5.将消息转发给以senderAccount登陆的设备
            chatMessage.set_messageid(messageID_sender);

            auto loginDeviceQueryService = this->mLoginDeviceQueryServicePtr.lock();
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            if (loginDeviceQueryService == nullptr || queryConnectionWithSessionService == nullptr ||
                clusterService == nullptr) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少loginDeviceQueryService、queryConnectionWithSessionService、clusterService");
                return;
            }
            auto itPair = loginDeviceQueryService->queryLoginDeviceSetWithUserAccount(senderAccount);

            if (itPair.first != itPair.second) {
                for (auto loginDeviceIt = itPair.first; loginDeviceIt != itPair.second; ++loginDeviceIt) {
                    if (kakaIM::Node::OnlineStateMessage_OnlineState_Online == loginDeviceIt->second ||
                        kakaIM::Node::OnlineStateMessage_OnlineState_Invisible == loginDeviceIt->second) {
                        if (loginDeviceIt->first.first ==
                            LoginDeviceQueryService::IDType_SessionID) {//设备是在此Server上登陆的

                            const std::string sessionID = loginDeviceIt->first.second;
                            if (sessionID == senderDeviceSessionID) {
                                continue;
                            }
                            chatMessage.set_sessionid(sessionID);
                            const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                                    sessionID);
                            if (!deviceConnectionIdentifier.empty()) {
                                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                                             chatMessage);
                                }
                            }
                        } else if (loginDeviceIt->first.first ==
                                   LoginDeviceQueryService::IDType_ServerID) {//设备在其它Server上登陆
                            const std::string serverID = loginDeviceIt->first.second;
                            kakaIM::president::ServerMessage serverMessage;
                            serverMessage.set_serverid(serverID);
                            serverMessage.set_messagetype(chatMessage.GetTypeName());
                            serverMessage.set_content(chatMessage.SerializeAsString());
                            serverMessage.set_targetuser(senderAccount);
                            assert(serverMessage.IsInitialized());
                            clusterService->sendServerMessage(serverMessage);
                        }
                    }
                }
            }
        }

        void
        SingleChatModule::addChatMessage(const kakaIM::Node::ChatMessage &message,
                                         const std::string connectionIdentifier) {
            //kakaIM::Node::ChatMessage *chatMessage = new kakaIM::Node::ChatMessage(message);
            std::unique_ptr<kakaIM::Node::ChatMessage> chatMessage(new kakaIM::Node::ChatMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(chatMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }
    }
}

