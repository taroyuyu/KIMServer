//
// Created by Kakawater on 2018/1/17.
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
#include "../../Common/proto/KakaIMMessage.pb.h"

namespace kakaIM {
    namespace node {
        void SessionModule::dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, std::string> & task){
            auto messageType = task.first->GetTypeName();
            if (messageType ==
                kakaIM::Node::RequestSessionIDMessage::default_instance().GetTypeName()) {
                handleSessionIDRequestMessage(
                        *(kakaIM::Node::RequestSessionIDMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::LogoutMessage::default_instance().GetTypeName()) {
                handleLogoutMessage(*(kakaIM::Node::LogoutMessage *) task.first.get(),
                                    task.second);
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
            if (message.GetTypeName() == kakaIM::Node::RequestSessionIDMessage::default_instance().GetTypeName() || message.GetTypeName() == kakaIM::Node::RegisterMessage::default_instance().GetTypeName()) {
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

        SessionModule::SessionModule() :KIMNodeModule(SessionModuleLogger){
        }
    }
}
