//
// Created by Kakawater on 2018/1/7.
//

#include <zconf.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "SingleChatModule.h"
#include "../Log/log.h"
#include "../../Common/util/Date.h"

namespace kakaIM {
    namespace node {
        SingleChatModule::SingleChatModule() : mEpollInstance(-1), messageEventfd(-1) {
            this->logger = log4cxx::Logger::getLogger(SingleChatModuleLogger);
        }

        SingleChatModule::~SingleChatModule() {
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
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (not this->m_needStop) {
                if (auto task = this->mTaskQueue.try_pop()) {
                    this->dispatchMessage(*task);
                } else {
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

        void SingleChatModule::shouldStop() {
            this->m_needStop = true;
        }

        void SingleChatModule::dispatchMessage(
                std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> &task) {
            auto messageType = task.first->GetTypeName();
            if (messageType == kakaIM::Node::ChatMessage::default_instance().GetTypeName()) {
                handleChatMessage(*(kakaIM::Node::ChatMessage *) task.first.get(),
                                  task.second);
            } else if (messageType ==
                       kakaIM::Node::VideoChatRequestMessage::default_instance().GetTypeName()) {
                handleVideoChatRequestMessage(
                        *(kakaIM::Node::VideoChatRequestMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::VideoChatRequestCancelMessage::default_instance().GetTypeName()) {
                handleVideoChatRequestCancelMessage(
                        *(kakaIM::Node::VideoChatRequestCancelMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::VideoChatReplyMessage::default_instance().GetTypeName()) {
                handleVideoChatReplyMessage(
                        *(kakaIM::Node::VideoChatReplyMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::VideoChatOfferMessage::default_instance().GetTypeName()) {
                handleVideoChatOfferMessage(
                        *(kakaIM::Node::VideoChatOfferMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::VideoChatAnswerMessage::default_instance().GetTypeName()) {
                handleVideoChatAnswerMessage(
                        *(kakaIM::Node::VideoChatAnswerMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::VideoChatNegotiationResultMessage::default_instance().GetTypeName()) {
                handleVideoChatNegotiationResultMessage(
                        *(kakaIM::Node::VideoChatNegotiationResultMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::VideoChatCandidateAddressMessage::default_instance().GetTypeName()) {
                handleVideoChatCandidateAddressMessage(
                        *(kakaIM::Node::VideoChatCandidateAddressMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::VideoChatByeMessage::default_instance().GetTypeName()) {
                handleVideoChatByeMessage(
                        *(kakaIM::Node::VideoChatByeMessage *) task.first.get(),
                        task.second);
            }
        }

        void SingleChatModule::handleChatMessage(kakaIM::Node::ChatMessage &chatMessage,
                                                 const std::string connectionIdentifier) {
            chatMessage.set_timestamp(kaka::Date::getCurrentDate().toString());
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
            messageSendService->sendMessageToUser(senderAccount, chatMessage);

        }

        void
        SingleChatModule::handleVideoChatRequestMessage(kakaIM::Node::VideoChatRequestMessage &videoChatRequestMessage,
                                                        const std::string connectionIdentifier) {
            auto clusterService = this->mClusterServicePtr.lock();
            auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock();
            auto userRelationService = this->mUserRelationServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();

            if (!(clusterService && queryUserAccountService && userRelationService && messageSendService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少clusterService、mqueryUserAccountService、userRelationService或messageSendService");
                return;
            }

            const std::string targetAccount = videoChatRequestMessage.targetaccount();

            //1.验证消息发送者
            const std::string sponsorAccount = queryUserAccountService->queryUserAccountWithSession(
                    videoChatRequestMessage.sessionid());

            if (sponsorAccount == targetAccount) {
                return;
            }

            const std::string sponsorsessionid = videoChatRequestMessage.sessionid();

            //2.验证提议着和目标用户是否存在好友关系
            if (false == userRelationService->checkFriendRelation(sponsorAccount, targetAccount)) {//不存在好友关系
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 用户:" << sponsorAccount << " 和用户:" << targetAccount
                                                         << "之间不存在好友关系,拒绝转发");
                return;
            }

            //3.将此通话提议保存到数据库
            uint64_t offerId = 0;
            std::string submissionTime;
            auto result = this->saveVideoChatOffer(sponsorAccount, targetAccount, clusterService->getServerID(),
                                                   videoChatRequestMessage.sessionid(), &offerId, &submissionTime);
            if (result != SaveVideoChatOfferResult_Success) {
                return;
            }
            videoChatRequestMessage.set_offerid(offerId);
            videoChatRequestMessage.set_sponsoraccount(sponsorAccount);
            videoChatRequestMessage.set_sponsorsessionid(sponsorsessionid);
            videoChatRequestMessage.set_timestamp(kaka::Date(submissionTime).toString());
            //4.将请求推送给双方
            messageSendService->sendMessageToUser(targetAccount, videoChatRequestMessage);
            messageSendService->sendMessageToUser(sponsorAccount, videoChatRequestMessage);
        }

        void SingleChatModule::handleVideoChatRequestCancelMessage(
                kakaIM::Node::VideoChatRequestCancelMessage &videoChatRequestCancelMessage,
                const std::string connectionIdentifier) {
            auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();

            if (!(clusterService && queryUserAccountService && messageSendService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少clusterService、mqueryUserAccountService或messageSendService");
                return;
            }

            //1.查询SessionID对应的userAccount
            const std::string userAccount = queryUserAccountService->queryUserAccountWithSession(
                    videoChatRequestCancelMessage.sessionid());

            //2.获取此通话提议的信息
            VideoChatOfferInfo offerInfo;
            auto result = this->fetchVideoChatOffer(videoChatRequestCancelMessage.offerid(), offerInfo);
            if (FetchVideoChatOfferResult_Success != result) {
                return;
            }

            //3.验证是否为此通话提议的发起方
            if (offerInfo.sponsorServerId != clusterService->getServerID() ||
                offerInfo.sponsorSessionId != videoChatRequestCancelMessage.sessionid()) {
                return;
            }

            //4.检查此通话提议的是否已经被受理
            if (VideoChatOfferState_Pending != offerInfo.state) {
                return;
            }

            //5.更改此通话提议的状态
            this->updateVideoChatOffer(offerInfo.offerId, "", "", VideoChatOfferState_Cancel);

            //6.该此通话取消的消息发送给接收方
            messageSendService->sendMessageToUser(offerInfo.targetAccount, videoChatRequestCancelMessage);
            //7.将此通话取消的消息发送给发送方
            messageSendService->sendMessageToUser(offerInfo.sponsorAccount, videoChatRequestCancelMessage);
        }

        void SingleChatModule::handleVideoChatReplyMessage(kakaIM::Node::VideoChatReplyMessage &videoChatReplyMessage,
                                                           const std::string connectionIdentifier) {
            videoChatReplyMessage.set_timestamp(kaka::Date::getCurrentDate().toString());
            auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            if (!(clusterService && queryUserAccountService && messageSendService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少clusterService、mqueryUserAccountService或messageSendService");
                return;
            }


            //1.获取此通话提议的信息
            VideoChatOfferInfo offerInfo;
            auto result = this->fetchVideoChatOffer(videoChatReplyMessage.offerid(), offerInfo);
            if (FetchVideoChatOfferResult_Success != result) {
                return;
            }
            //2.验证消息发送者
            const std::string userAccount = queryUserAccountService->queryUserAccountWithSession(
                    videoChatReplyMessage.sessionid());

            if (userAccount.empty() || userAccount != offerInfo.targetAccount) {
                return;
            }
            //3.检查此通话提议的是否已经被受理
            if (VideoChatOfferState_Pending != offerInfo.state) {
                return;
            }

            //4.修改此通话提议的状态
            switch (videoChatReplyMessage.reply()) {
                case kakaIM::Node::VideoChatReplyMessage_VideoChatReply_VideoChatReply_Allow: {
                    this->updateVideoChatOffer(videoChatReplyMessage.offerid(), clusterService->getServerID(),
                                               videoChatReplyMessage.sessionid(), VideoChatOfferState_Accept);
                }
                    break;
                case kakaIM::Node::VideoChatReplyMessage_VideoChatReply_VideoChatReply_Reject: {
                    this->updateVideoChatOffer(videoChatReplyMessage.offerid(), clusterService->getServerID(),
                                               videoChatReplyMessage.sessionid(), VideoChatOfferState_Reject);
                }
                    break;
                case kakaIM::Node::VideoChatReplyMessage_VideoChatReply_VideoChatReply_NoAnswer: {
                    this->updateVideoChatOffer(videoChatReplyMessage.offerid(), "", "", VideoChatOfferState_NoAnswer);
                }
                    break;
                default: {
                    return;
                }
            }

            videoChatReplyMessage.set_sponsoraccount(offerInfo.sponsorAccount);
            videoChatReplyMessage.set_sponsorsessionid(offerInfo.sponsorSessionId);
            videoChatReplyMessage.set_targetaccount(offerInfo.targetAccount);
            videoChatReplyMessage.set_answersessionid(videoChatReplyMessage.sessionid());


            //5.将请求推送给双方
            messageSendService->sendMessageToUser(offerInfo.targetAccount, videoChatReplyMessage);
            messageSendService->sendMessageToUser(offerInfo.sponsorAccount, videoChatReplyMessage);
        }

        void SingleChatModule::handleVideoChatOfferMessage(kakaIM::Node::VideoChatOfferMessage &videoChatOfferMessage,
                                                           const std::string connectionIdentifier) {
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            if (!(queryConnectionWithSessionService && connectionOperationService && clusterService &&
                  messageSendService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少queryConnectionWithSessionService、connectionOperationService、clusterService或messageSendService");
                return;
            }

            //1.获取此通话提议的信息
            VideoChatOfferInfo offerInfo;
            auto result = this->fetchVideoChatOffer(videoChatOfferMessage.offerid(), offerInfo);
            if (FetchVideoChatOfferResult_Success != result) {
                return;
            }

            //2.检查此通话提议的状态
            if (VideoChatOfferState_Accept != offerInfo.state) {
                return;
            }

            //3.检查通信双方是否匹配
            if (offerInfo.sponsorSessionId != videoChatOfferMessage.sessionid()) {
                return;
            }

            //4.将此消息发送给接听者

            if (offerInfo.recipientServerId == clusterService->getServerID()) {
                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                        offerInfo.recipientSessionId);
                if (!deviceConnectionIdentifier.empty()) {
                    videoChatOfferMessage.set_sessionid(offerInfo.recipientSessionId);
                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                             videoChatOfferMessage);
                } else {
                    const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                            videoChatOfferMessage.sessionid());
                    if (!deviceConnectionIdentifier.empty()) {
                        kakaIM::Node::VideoChatByeMessage byeMessage;
                        byeMessage.set_offerid(videoChatOfferMessage.offerid());
                        byeMessage.set_sessionid(videoChatOfferMessage.sessionid());
                        connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                                 byeMessage);
                    }
                }
            } else {
                messageSendService->sendMessageToSession(offerInfo.recipientServerId, offerInfo.recipientSessionId,
                                                         videoChatOfferMessage);
            }

        }

        void
        SingleChatModule::handleVideoChatAnswerMessage(kakaIM::Node::VideoChatAnswerMessage &videoChatAnswerMessage,
                                                       const std::string connectionIdentifier) {
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            if (!(queryConnectionWithSessionService && connectionOperationService && clusterService &&
                  messageSendService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少queryConnectionWithSessionService、connectionOperationService、clusterService或messageSendService");
                return;
            }

            //1.获取此通话提议的信息
            VideoChatOfferInfo offerInfo;
            auto result = this->fetchVideoChatOffer(videoChatAnswerMessage.offerid(), offerInfo);
            if (FetchVideoChatOfferResult_Success != result) {
                return;
            }

            //2.检查此通话提议的状态
            if (VideoChatOfferState_Accept != offerInfo.state) {
                return;
            }

            //3.检查通信双方是否匹配
            if (offerInfo.recipientSessionId != videoChatAnswerMessage.sessionid()) {
                return;
            }

            //4.将此消息发送给发起者
            if (offerInfo.sponsorServerId == clusterService->getServerID()) {
                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                        offerInfo.sponsorSessionId);
                if (!deviceConnectionIdentifier.empty()) {
                    videoChatAnswerMessage.set_sessionid(offerInfo.sponsorSessionId);
                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                             videoChatAnswerMessage);
                } else {
                    const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                            videoChatAnswerMessage.sessionid());
                    if (!deviceConnectionIdentifier.empty()) {
                        kakaIM::Node::VideoChatByeMessage byeMessage;
                        byeMessage.set_offerid(videoChatAnswerMessage.offerid());
                        byeMessage.set_sessionid(videoChatAnswerMessage.sessionid());
                        connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                                 byeMessage);
                    }
                }
            } else {
                messageSendService->sendMessageToSession(offerInfo.sponsorServerId, offerInfo.sponsorSessionId,
                                                         videoChatAnswerMessage);
            }

        }

        void SingleChatModule::handleVideoChatNegotiationResultMessage(
                kakaIM::Node::VideoChatNegotiationResultMessage &videoChatNegotiationResultMessage,
                const std::string connectionIdentifier) {
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            if (!(queryConnectionWithSessionService && connectionOperationService && clusterService &&
                  messageSendService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少queryConnectionWithSessionService、connectionOperationService、clusterService或messageSendService");
                return;
            }

            //1.获取此通话提议的信息
            VideoChatOfferInfo offerInfo;
            auto result = this->fetchVideoChatOffer(videoChatNegotiationResultMessage.offerid(), offerInfo);
            if (FetchVideoChatOfferResult_Success != result) {
                return;
            }

            //2.检查此通话提议的状态
            if (VideoChatOfferState_Accept != offerInfo.state) {
                return;
            }

            //3.检查通信双方是否匹配
            std::string opponentSessionId;
            std::string opponentServerId;
            if (offerInfo.sponsorSessionId == videoChatNegotiationResultMessage.sessionid()) {
                opponentSessionId = offerInfo.recipientSessionId;
                opponentServerId = offerInfo.recipientServerId;
            } else if (offerInfo.recipientSessionId == videoChatNegotiationResultMessage.sessionid()) {
                opponentSessionId = offerInfo.sponsorSessionId;
                opponentServerId = offerInfo.sponsorServerId;
            } else {
                return;
            }

            //4.将此消息发送给对方
            if (opponentServerId == clusterService->getServerID()) {
                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                        opponentSessionId);
                if (!deviceConnectionIdentifier.empty()) {
                    videoChatNegotiationResultMessage.set_sessionid(opponentSessionId);
                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                             videoChatNegotiationResultMessage);
                } else {
                    const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                            videoChatNegotiationResultMessage.sessionid());
                    if (!deviceConnectionIdentifier.empty()) {
                        kakaIM::Node::VideoChatByeMessage byeMessage;
                        byeMessage.set_offerid(videoChatNegotiationResultMessage.offerid());
                        byeMessage.set_sessionid(videoChatNegotiationResultMessage.sessionid());
                        connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                                 byeMessage);
                    }
                }
            } else {
                messageSendService->sendMessageToSession(opponentServerId, opponentSessionId,
                                                         videoChatNegotiationResultMessage);
            }
        }

        void SingleChatModule::handleVideoChatCandidateAddressMessage(
                kakaIM::Node::VideoChatCandidateAddressMessage &videoChatCandidateAddressMessage,
                const std::string connectionIdentifier) {
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            if (!(queryConnectionWithSessionService && connectionOperationService && clusterService &&
                  messageSendService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少queryConnectionWithSessionService、connectionOperationService、clusterService或messageSendService");
                return;
            }

            //1.获取此通话提议的信息
            VideoChatOfferInfo offerInfo;
            auto result = this->fetchVideoChatOffer(videoChatCandidateAddressMessage.offerid(), offerInfo);
            if (FetchVideoChatOfferResult_Success != result) {
                return;
            }

            //2.检查此通话提议的状态
            if (VideoChatOfferState_Accept != offerInfo.state) {
                return;
            }

            //3.检查通信双方是否匹配
            std::string opponentSessionId;
            std::string opponentServerId;
            if (offerInfo.sponsorSessionId == videoChatCandidateAddressMessage.sessionid()) {
                opponentSessionId = offerInfo.recipientSessionId;
                opponentServerId = offerInfo.recipientServerId;
            } else if (offerInfo.recipientSessionId == videoChatCandidateAddressMessage.sessionid()) {
                opponentSessionId = offerInfo.sponsorSessionId;
                opponentServerId = offerInfo.sponsorServerId;
            } else {
                return;
            }

            if (videoChatCandidateAddressMessage.sessionid() != videoChatCandidateAddressMessage.fromsessionid()) {
                return;
            }

            //4.将此消息发送给对方
            if (opponentServerId == clusterService->getServerID()) {
                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                        opponentSessionId);
                if (!deviceConnectionIdentifier.empty()) {
                    videoChatCandidateAddressMessage.set_sessionid(opponentSessionId);
                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                             videoChatCandidateAddressMessage);
                } else {
                    const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                            videoChatCandidateAddressMessage.sessionid());
                    if (!deviceConnectionIdentifier.empty()) {
                        kakaIM::Node::VideoChatByeMessage byeMessage;
                        byeMessage.set_offerid(videoChatCandidateAddressMessage.offerid());
                        byeMessage.set_sessionid(videoChatCandidateAddressMessage.sessionid());
                        connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                                 byeMessage);
                    }
                }
            } else {
                messageSendService->sendMessageToSession(opponentServerId, opponentSessionId,
                                                         videoChatCandidateAddressMessage);
            }
        }

        void SingleChatModule::handleVideoChatByeMessage(kakaIM::Node::VideoChatByeMessage &videoChatByeMessage,
                                                         const std::string connectionIdentifier) {
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            if (!(queryConnectionWithSessionService && connectionOperationService && clusterService &&
                  messageSendService)) {
                LOG4CXX_ERROR(this->logger, __FUNCTION__
                        << "无法执行，缺少queryConnectionWithSessionService、connectionOperationService、clusterService或messageSendService");
                return;
            }

            //1.获取此通话提议的信息
            VideoChatOfferInfo offerInfo;
            auto result = this->fetchVideoChatOffer(videoChatByeMessage.offerid(), offerInfo);
            if (FetchVideoChatOfferResult_Success != result) {
                return;
            }

            //2.检查此通话提议的状态
            if (VideoChatOfferState_Accept != offerInfo.state) {
                return;
            }

            //3.检查通信双方是否匹配
            std::string opponentSessionId;
            std::string opponentServerId;
            if (offerInfo.sponsorSessionId == videoChatByeMessage.sessionid()) {
                opponentSessionId = offerInfo.recipientSessionId;
                opponentServerId = offerInfo.recipientServerId;
            } else if (offerInfo.recipientSessionId == videoChatByeMessage.sessionid()) {
                opponentSessionId = offerInfo.sponsorSessionId;
                opponentServerId = offerInfo.sponsorServerId;
            } else {
                return;
            }


            //4.将此消息发送给对方
            if (opponentServerId == clusterService->getServerID()) {
                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                        opponentSessionId);
                if (!deviceConnectionIdentifier.empty()) {
                    videoChatByeMessage.set_sessionid(opponentSessionId);
                    connectionOperationService->sendMessageThroughConnection(deviceConnectionIdentifier,
                                                                             videoChatByeMessage);
                }
            } else {
                messageSendService->sendMessageToSession(opponentServerId, opponentSessionId, videoChatByeMessage);
            }

        }

        SingleChatModule::SaveVideoChatOfferResult
        SingleChatModule::saveVideoChatOffer(const std::string spnsorAccount, const std::string targetAccount,
                                             const std::string sponsorServer, const std::string sponsorSessionId,
                                             uint64_t *offerId, std::string *submissionTime) {
            const std::string SaveVideoChatOfferStatement = "SaveVideoChatOffer";
            const std::string saveVideoChatOfferSQL = "INSERT INTO video_chat_offer_record(offerid, spnsor, target, sponsor_server, sponsor_session, state, submission_time)\n"
                                                      "VALUES(DEFAULT,$1,$2,$3,$4,DEFAULT ,now()) RETURNING offerid,submission_time;";
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                //异常处理
                return SaveVideoChatOfferResult_DBConnectionNotExit;;
            }

            try {
                pqxx::work dbWork(*dbConnection);

                auto saveVideoChatOfferStatementInvocation = dbWork.prepared(
                        SaveVideoChatOfferStatement);

                if (!saveVideoChatOfferStatementInvocation.exists()) {
                    dbConnection->prepare(SaveVideoChatOfferStatement, saveVideoChatOfferSQL);
                }

                pqxx::result result = saveVideoChatOfferStatementInvocation(spnsorAccount)(
                        targetAccount)(sponsorServer)(sponsorSessionId).exec();


                //提交事务
                dbWork.commit();

                *offerId = result[0][0].as<uint64_t>();
                *submissionTime = result[0][1].as<std::string>();

                return SaveVideoChatOfferResult_Success;

            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "将通话提议插入数据库失败,"
                                                  << exception.what());

                return SaveVideoChatOfferResult_InteralError;
            }

        }

        SingleChatModule::FetchVideoChatOfferResult
        SingleChatModule::fetchVideoChatOffer(const uint64_t offerId, VideoChatOfferInfo &offerInfo) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                //异常处理
                return FetchVideoChatOfferResult_DBConnectionNotExit;
            }

            try {
                pqxx::work dbWork(*dbConnection);

                static const std::string QueryVideoChatOfferInfoStatement = "QueryVideoChatOfferInfo";
                static const std::string queryVideoChatOfferInfoSQL = "SELECT spnsor,target,sponsor_server,sponsor_session,recipient_server,recipient_session,state,submission_time FROM video_chat_offer_record WHERE offerid = $1;";

                auto queryVideoChatOfferInfoStatementInvocation = dbWork.prepared(QueryVideoChatOfferInfoStatement);
                if (!queryVideoChatOfferInfoStatementInvocation.exists()) {
                    dbConnection->prepare(QueryVideoChatOfferInfoStatement, queryVideoChatOfferInfoSQL);
                }

                pqxx::result result = queryVideoChatOfferInfoStatementInvocation(offerId).exec();

                if (result.size()) {
                    offerInfo.sponsorAccount = result[0][0].as<std::string>();
                    offerInfo.targetAccount = result[0][1].as<std::string>();
                    offerInfo.sponsorServerId = result[0][2].as<std::string>();
                    offerInfo.sponsorSessionId = result[0][3].as<std::string>();
                    if (result[0][4].is_null()) {
                        offerInfo.recipientServerId = "";
                    } else {
                        offerInfo.recipientServerId = result[0][4].as<std::string>();
                    }
                    if (result[0][5].is_null()) {
                        offerInfo.recipientSessionId = "";
                    } else {
                        offerInfo.recipientSessionId = result[0][5].as<std::string>();
                    }
                    std::string state = result[0][6].as<std::string>();
                    if ("Pending" == state) {
                        offerInfo.state = VideoChatOfferState_Pending;
                    } else if ("Cancel" == state) {
                        offerInfo.state = VideoChatOfferState_Cancel;
                    } else if ("Allow" == state) {
                        offerInfo.state = VideoChatOfferState_Accept;
                    } else if ("Reject" == state) {
                        offerInfo.state = VideoChatOfferState_Reject;
                    } else {
                        LOG4CXX_ERROR(this->logger, typeid(this).name() << " " << __FUNCTION__ << " 查询视频通话提议，offerId="
                                                                        << offerId << "的信息失败，由于state的类型不能识别，state="
                                                                        << state);
                        return FetchVideoChatOfferResult_InteralError;
                    }
                    offerInfo.submissionTime = result[0][7].as<std::string>();
                    return FetchVideoChatOfferResult_Success;
                } else {
                    return FetchVideoChatOfferResult_DBConnectionNotExit;
                }

            } catch (std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 查询视频通话提议，offerId" << offerId
                                                  << "的信息失败," << exception.what());
                return FetchVideoChatOfferResult_InteralError;
            }
        }

        SingleChatModule::UpdateVideoChatOfferResult
        SingleChatModule::updateVideoChatOffer(const uint64_t offerId, std::string recipientServerId,
                                               std::string recipientSessionId, const VideoChatOfferState offerState) {

            std::string state = "Pending";
            switch (offerState) {
                case VideoChatOfferState_Pending: {
                    state = "Pending";
                }
                    break;
                case VideoChatOfferState_Cancel: {
                    state = "Cancel";
                }
                    break;
                case VideoChatOfferState_Accept: {
                    state = "Allow";
                }
                    break;
                case VideoChatOfferState_Reject: {
                    state = "Reject";
                }
                    break;
                default: {
                    return UpdateVideoChatOfferResult_ParameterErrpr;
                }
            }


            static const std::string UpdateVideoChatOfferStatement = "UpdateVideoChatOffer";
            static const std::string updateVideoChatOfferSQL = "UPDATE video_chat_offer_record SET state = $2,recipient_server = $3,recipient_session = $4 WHERE offerid = $1;";
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                //异常处理
                return UpdateVideoChatOfferResult_DBConnectionNotExit;;
            }

            try {
                pqxx::work dbWork(*dbConnection);

                auto updateVideoChatOfferStatementInvocation = dbWork.prepared(
                        UpdateVideoChatOfferStatement);

                if (!updateVideoChatOfferStatementInvocation.exists()) {
                    dbConnection->prepare(UpdateVideoChatOfferStatement, updateVideoChatOfferSQL);
                }

                pqxx::result result = updateVideoChatOfferStatementInvocation(offerId)(state)(recipientServerId)(
                        recipientSessionId).exec();


                //提交事务
                dbWork.commit();

                return UpdateVideoChatOfferResult_Success;

            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "更新通话提议状态失败," << exception.what());

                return UpdateVideoChatOfferResult_Failed;
            }
        }

        std::shared_ptr<pqxx::connection> SingleChatModule::getDBConnection() {
            if (this->m_dbConnection) {
                return this->m_dbConnection;
            }

            const std::string postgrelConnectionUrl =
                    "dbname=" + this->dbConfig.getDBName() + " user=" + this->dbConfig.getUserAccount() + " password=" +
                    this->dbConfig.getUserPassword() + " hostaddr=" + this->dbConfig.getHostAddr() + " port=" +
                    std::to_string(this->dbConfig.getPort());

            try {

                this->m_dbConnection = std::make_shared<pqxx::connection>(postgrelConnectionUrl);

                if (!this->m_dbConnection->is_open()) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "打开数据库失败");
                }


            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "连接数据库出错," << exception.what());
            }

            return this->m_dbConnection;
        }
    }
}

