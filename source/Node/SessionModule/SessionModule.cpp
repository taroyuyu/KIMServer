//
// Created by taroyuyu on 2018/1/17.
//

#include <zconf.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sstream>
#include <uuid/uuid.h>
#include "../Events/UserLogoutEvent.h"
#include "SessionModule.h"
#include "../../Common/EventBus/EventBus.h"
#include "../../Common/util/MD5.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace node {
        bool SessionModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, ::EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }

            //创建Epoll实例
            if (-1 == (this->epollInstance = epoll_create1(0))) {
                return false;
            }

            //向Epoll实例注册messageEventfd事件
            struct epoll_event messageEventfdEvent;
            messageEventfdEvent.events = EPOLLIN;
            messageEventfdEvent.data.fd = this->messageEventfd;
            if (-1 == epoll_ctl(this->epollInstance, EPOLL_CTL_ADD, this->messageEventfd, &messageEventfdEvent)) {
                return false;
            }

            return true;
        }

        void SessionModule::execute() {
            while (this->m_isStarted) {

                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为0.1秒
                int happedEventsCount = epoll_wait(this->epollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   1000);

                if (-1 == happedEventsCount) {
                    LOG4CXX_WARN(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 等待Epill实例上的事件出错");
                }

                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (EPOLLIN & happedEvents[i].events) {
                        if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->messageQueue.empty()) {
                                    this->messageQueueMutex.lock();
                                    auto pairIt = std::move(this->messageQueue.front());
                                    this->messageQueue.pop();
                                    this->messageQueueMutex.unlock();

                                    auto messageType = pairIt.first->GetTypeName();
                                    if (messageType ==
                                        kakaIM::Node::RequestSessionIDMessage::default_instance().GetTypeName()) {
                                        handleSessionIDRequestMessage(
                                                *(kakaIM::Node::RequestSessionIDMessage *) pairIt.first.get(),
                                                pairIt.second);
                                    } else if (messageType ==
                                               kakaIM::Node::LogoutMessage::default_instance().GetTypeName()) {
                                        handleLogoutMessage(*(kakaIM::Node::LogoutMessage *) pairIt.first.get(),
                                                            pairIt.second);
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
        }

        bool SessionModule::doFilter(const ::google::protobuf::Message &message,
                                     const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            if (message.GetTypeName() == kakaIM::Node::HeartBeatMessage::default_instance().GetTypeName()) {
                LOG4CXX_TRACE(this->logger,"connectionIdentifier="<<connectionIdentifier<<"上收到心跳消息");
                return false;
            } else
                //若是请求SessionID则直接放行通过
            if (message.GetTypeName() == kakaIM::Node::RequestSessionIDMessage::default_instance().GetTypeName()) {
                return true;
            } else {//其余消息
                //1.获取sessionID
                std::string sessionID = message.GetReflection()->GetString(message,
                                                                           message.GetDescriptor()->FindFieldByName(
                                                                                   "sessionID"));
                //2.检验sessionID对应的connection是否与发送此消息的connection是否相同
                auto pairIt = this->sessionMap.find(sessionID);
                //不存在此sessionID
                if (pairIt == this->sessionMap.end()) {
                    return false;
                } else {
                    //连接sessionID存在
                    if (pairIt->second == connectionIdentifier) {//是同一连接
                        return true;
                    } else {//不是同一连接
                        return false;
                    }
                }

            }
        }

        void SessionModule::didCloseConnection(const std::string connectionIdentifier) {
            auto connectionPairIt = std::find_if(this->sessionMap.begin(), this->sessionMap.end(),
                                                 [connectionIdentifier](
                                                         const std::map<std::string, std::string>::value_type &item) -> bool {
                                                     return item.second == connectionIdentifier;
                                                 });
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            if (connectionPairIt != this->sessionMap.end()) {
                std::string sessionID = connectionPairIt->first;
                this->sessionMap.erase(connectionPairIt);
            }
        }

        void SessionModule::addSessionIDRequesMessage(const kakaIM::Node::RequestSessionIDMessage &message,
                                                      const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            std::unique_ptr<kakaIM::Node::RequestSessionIDMessage> requestSessionIDMessage(
                    new kakaIM::Node::RequestSessionIDMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(
                    std::move(requestSessionIDMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void SessionModule::addLogoutMessage(const kakaIM::Node::LogoutMessage &message,
                                             const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            std::unique_ptr<kakaIM::Node::LogoutMessage> logoutMessage(new kakaIM::Node::LogoutMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(
                    std::move(logoutMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void SessionModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void SessionModule::handleSessionIDRequestMessage(const kakaIM::Node::RequestSessionIDMessage &message,
                                                          const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            //1.生成sessionID
            std::string sessionID = this->generateSessionID(connectionIdentifier);
            if (this->sessionMap.find(sessionID) == this->sessionMap.end()) {
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 生成sessionID=" << sessionID);
                //2.绑定sessionID和connection
                this->sessionMap.emplace(sessionID, connectionIdentifier);
                //3.生成Response
                kakaIM::Node::ResponseSessionIDMessage responseSessionIDMessage;
                responseSessionIDMessage.set_sessionid(sessionID);
                responseSessionIDMessage.set_status(Node::ResponseSessionIDMessage_Status_Success);
                //4.发送Response
                assert(responseSessionIDMessage.IsInitialized());
                LOG4CXX_DEBUG(this->logger, __FUNCTION__ << " 发送ResponseSessionIDMessage");
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                             responseSessionIDMessage);
                } else {
                    LOG4CXX_ERROR(this->logger,
                                  __FUNCTION__ << " 无法发送ResponseSessionIDMessage，由于connectionOperationService不存在");
                }
            } else {
                kakaIM::Node::ResponseSessionIDMessage responseSessionIDMessage;
                responseSessionIDMessage.set_sessionid(sessionID);
                responseSessionIDMessage.set_status(Node::ResponseSessionIDMessage_Status_Success);
                assert(responseSessionIDMessage.IsInitialized());
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                             responseSessionIDMessage);
                }
            }
        }

        void
        SessionModule::handleLogoutMessage(const kakaIM::Node::LogoutMessage &message,
                                           const std::string connectionIdentifier) {
            //1.获取用户账号
            std::string sessionID = message.sessionid();
            //2.销毁SessionID
            this->sessionMap.erase(sessionID);
            //3.生成Response
            kakaIM::Node::ResponseLogoutMessage responseLogoutMessage;
            responseLogoutMessage.set_sessionid(sessionID);
            responseLogoutMessage.set_offlinemailestate(kakaIM::Node::ResponseLogoutMessage_OfflineMailState_Close);
            //4.发送Response
            if (responseLogoutMessage.IsInitialized()) {
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, message);
                }
            }
            //5.发送通知
            //UserLogoutEvent * userLogoutEvent = new UserLogoutEvent(sessionID);
            //EventBus::getDefault().Post(userLogoutEvent);
            //userLogoutEvent->release();
        }

        std::string SessionModule::generateSessionID(const std::string connectionIdentifier) {
            //1.为此连接生成uuid
            uuid_t connectionUUID;
            char connectionUUID_str[37];
            uuid_generate_random(connectionUUID);
            uuid_unparse(connectionUUID, connectionUUID_str);
            uuid_clear(connectionUUID);
            //2.获取当前时间
            kaka::Date currentDate = kaka::Date::getCurrentDate();
            //3.拼接uuid和currentDate,取摘要
            return ::MD5(currentDate.toString() + connectionUUID_str).toStr();
        }

        SessionModule::SessionModule() :epollInstance(-1), messageEventfd(-1){
            this->logger = log4cxx::Logger::getLogger(SessionModuleLogger);
        }

        SessionModule::~SessionModule() {
        }
    }
}
