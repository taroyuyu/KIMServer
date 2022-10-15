//
// Created by Kakawater on 2018/1/29.
//

#include <zconf.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <Client/SessionModule/SessionModule.h>
#include <Common/proto/KakaIMMessage.pb.h>
#include <Common/util/Date.h>

namespace kakaIM {
    namespace client {
        SessionModule::SessionModule(std::string serverAddr, uint32_t serverPort) :
                mEpollInstance(-1),mInputMessageEventfd(-1),mOutputMessageEventfd(-1),mSocketManager(new net::TCPSocketManager()),mKakaIMMessageAdapter(new common::KakaIMMessageAdapter()), serverAddr(serverAddr), serverPort(serverPort),
                state(SessionModuleState_UNConnect),mHeartBeatTimerfd(-1) {
            this->mSocketManager->start();
        }

        bool SessionModule::init() {
            //创建Epoll实例
            if (-1 == (this->mEpollInstance = epoll_create1(0))) {
                std::cout << "SessionModule() mEpollInstance创建失败" << std::endl;
                return false;
            }

            this->mInputMessageEventfd = eventfd(0, EFD_SEMAPHORE);
            if (this->mInputMessageEventfd < 0) {
                std::cout << "SessionModule() mInputMessageEventfdEventfd创建失败" << std::endl;
                return false;
            }

            //交由Epoll实例进行托管
            struct epoll_event inputMessageEvent;
            inputMessageEvent.events = EPOLLIN;
            inputMessageEvent.data.fd = this->mInputMessageEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->mInputMessageEventfd, &inputMessageEvent)) {
                perror("SessionModule()向Epoll实例注册mInputMessageEventfd失败");
                return false;
            }

            this->mOutputMessageEventfd = eventfd(0, EFD_SEMAPHORE);
            if (this->mOutputMessageEventfd < 0) {
                std::cout << "SessionModule() mOutputMessageEventfd创建失败" << std::endl;
                return false;
            }

            //交由Epoll实例进行托管
            struct epoll_event outputMessageEvent;
            outputMessageEvent.events = EPOLLIN;
            outputMessageEvent.data.fd = this->mOutputMessageEventfd;
            if (-1 ==
                epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->mOutputMessageEventfd, &outputMessageEvent)) {
                perror("SessionModule()向Epoll实例注册mOutputMessageEventfd失败");
                return false;
            }

            return true;
        }

        SessionModule::~SessionModule() {
            this->stop();
            if (this->mKakaIMMessageAdapter) {
                delete this->mKakaIMMessageAdapter;
                this->mKakaIMMessageAdapter = nullptr;
            }
        }

        void SessionModule::sendMessage(const google::protobuf::Message &message) {
            //将消息插入到输入队列
            std::unique_ptr<::google::protobuf::Message> messageDuplicated(message.New());
            messageDuplicated->CopyFrom(message);
            this->mOutputMessageQueue.emplace(std::move(messageDuplicated));
            u_int64_t messageCount = 1;
            if (sizeof(messageCount) != ::write(this->mOutputMessageEventfd, &messageCount, sizeof(messageCount))) {
                std::cout << "SessionModule::" << __func__ << __LINE__ << "发送messageEvent失败" << std::endl;
            }
        }

        void SessionModule::addServiceModule(ServiceModule *serviceModule) {
            serviceModule->setSessionModule(*this);
            auto messageTypeList = serviceModule->messageTypes();
            for (auto it = messageTypeList.begin(); it != messageTypeList.end(); ++it) {
                std::string messageType = *it;
                auto moduleListIt = this->moduleList.find(messageType);

                if (moduleListIt != this->moduleList.end()) {
                    moduleListIt->second.emplace_back(serviceModule);
                } else {
                    std::list<ServiceModule *> moduleList;
                    moduleList.emplace_back(serviceModule);
                    this->moduleList.emplace(messageType, moduleList);
                }
            }
        }


        void SessionModule::execute() {
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }
            while (not this->m_needStop) {
                int const kHandleEventMaxCountPerLoop = 10;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   10000);

                if (-1 == happedEventsCount) {
                    perror("SessionModule::execute() 等待Epoll实例上的事件出错");
                    exit(EXIT_FAILURE);
                }

                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (this->mHeartBeatTimerfd == happedEvents[i].data.fd) {//定时器超时
                        u_int64_t expirationCount = 0;
                        if (int state = read(this->mHeartBeatTimerfd, &expirationCount, sizeof(expirationCount))) {
                            //发送心跳消息
                            Node::HeartBeatMessage heartBeatMessage;
                            heartBeatMessage.set_sessionid(this->currentSessionID);
                            heartBeatMessage.set_timestamp(kaka::Date::getCurrentDate().toString());
                            this->mSocketManager->sendMessage(this->mConnectionSocket, heartBeatMessage);
                        } else {
                            std::cout << "SessionModule::" << __func__ << "read(mHeartBeatTimerfd)出错，返回值:" << state
                                      << std::endl;
                        }
                    } else if (this->mInputMessageEventfd == happedEvents[i].data.fd) {//输入事件
                        u_int64_t messageCount = 0;
                        if (int state = read(this->mInputMessageEventfd, &messageCount, sizeof(messageCount))) {
                            while (messageCount--) {
                                if (!this->mInputMessageQueue.empty()) {
                                    auto message = std::move(this->mInputMessageQueue.front());
                                    this->mInputMessageQueue.pop();
                                    if (message) {
                                        this->handleInputMessage(*message);
                                    } else {
                                        std::cout << "SessionModule::" << __FUNCTION__
                                                  << " 从mInputMessageQueue中取出的message为nullptr" << std::endl;
                                    }
                                } else {
                                    break;
                                }
                            }
                        } else {
                            std::cout << "SessionModule::" << __func__ << "read(mInputMessageEventfd)出错，返回值:" << state
                                      << std::endl;
                        }
                    } else if (this->mOutputMessageEventfd == happedEvents[i].data.fd) {//输出事件
                        u_int64_t messageCount = 0;
                        if (int state = read(this->mOutputMessageEventfd, &messageCount, sizeof(messageCount))) {
                            while (messageCount--) {
                                if (!this->mOutputMessageQueue.empty()) {
                                    auto message = std::move(this->mOutputMessageQueue.front());
                                    this->mOutputMessageQueue.pop();
                                    if (message) {
                                        this->handleOutputMessage(*message);
                                    }
                                } else {
                                    break;
                                }
                            }
                        } else {
                            std::cout << "SessionModule::" << __func__ << "read(mOutputMessageEventfd)出错，返回值:" << state
                                      << std::endl;
                        }
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

        void SessionModule::buildSession(BuildSessionSuccessCallback success, BuildSessionFailureCallback failure) {
            if (SessionModuleState_Connected == state) {
                if (success) {
                    success(*this);
                }
                return;
            }
            this->buildSessionSuccessCallback = success;
            this->buildSessionFailureCallback = failure;
            //1.建立连接
            this->mConnectionSocket.connect(this->serverAddr, this->serverPort);
            //2.注册到SocketManager
            this->mSocketManager->addSocket(this->mConnectionSocket.getSocketfd(), this, this->mKakaIMMessageAdapter);
            //3.发送请求会话消息
            Node::RequestSessionIDMessage requestSessionIDMessage;
            this->mSocketManager->sendMessage(this->mConnectionSocket,requestSessionIDMessage);
            this->state = SessionModuleState_SessionBuilding;
        }

        void SessionModule::shouldStop(){
            this->m_needStop = true;
        }

        void SessionModule::destorySession() {

        }

        void SessionModule::didConnectionClosed(net::TCPClientSocket clientSocket) {
            //连接断开，启动重连机制
        }

        void SessionModule::handleInputMessage(::google::protobuf::Message &message) {
            //根据消息类型查找相应的module
            auto moduleListIt = this->moduleList.find(message.GetTypeName());
            if (moduleListIt != this->moduleList.end()) {
                for (auto moduleIt = moduleListIt->second.begin(); moduleIt != moduleListIt->second.end(); ++moduleIt) {
                    if (*moduleIt != nullptr) {
                        (*moduleIt)->handleMessage(message, *this);
                    } else {
                        std::cout << "SessionModule::" << __FUNCTION__ << "找到的Module为null,messageType="
                                  << message.GetTypeName() << std::endl;
                    }
                }
            } else {
                std::cout << "SessionModule::" << __FUNCTION__ << " 找不到能处理" << message.GetTypeName() << "的模块"
                          << std::endl;
            }
        }

        void SessionModule::handleOutputMessage(::google::protobuf::Message &message) {
            if (!this->currentSessionID.empty()) {
                message.GetReflection()->SetString(&message, message.GetDescriptor()->FindFieldByName("sessionID"),
                                                   this->currentSessionID);
                //发送消息
                this->mSocketManager->sendMessage(this->mConnectionSocket, message);
            }

        }

        void SessionModule::didReceivedMessage(int socketfd, const ::google::protobuf::Message &message) {
            if (message.GetTypeName() == Node::ResponseHeartBeatMessage::default_instance().GetTypeName()) {
//                const Node::ResponseHeartBeatMessage &responseHeartBeatMessage = *((const Node::ResponseHeartBeatMessage *) &message);

            } else if (message.GetTypeName() == Node::ResponseSessionIDMessage::default_instance().GetTypeName()) {

                if (this->state == SessionModuleState_SessionBuilding) {
                    const Node::ResponseSessionIDMessage &responseSessionIDMessage = *((const Node::ResponseSessionIDMessage *) &message);

                    if(responseSessionIDMessage.status() == Node::ResponseSessionIDMessage_Status_Success){
                        this->currentSessionID = responseSessionIDMessage.sessionid();

                        //1.启动心跳机制
                        if (-1 == this->mHeartBeatTimerfd) {//若定时器未启动，则启动定时器
                            this->mHeartBeatTimerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
                            if (-1 == this->mHeartBeatTimerfd) {
                                std::cout << "SessionModule::" << __func__ << "定时器创建失败,errno = " << errno << std::endl;
                            } else {
                                struct itimerspec timerConfig;
                                timerConfig.it_value.tv_sec = 1;//1秒之后超时
                                timerConfig.it_value.tv_nsec = 0;
                                timerConfig.it_interval.tv_sec = 5;//间隔周期也是1秒
                                timerConfig.it_interval.tv_nsec = 0;
                                timerfd_settime(this->mHeartBeatTimerfd, 0, &timerConfig, nullptr);//相对定时器
                                //交由Epoll实例进行托管
                                struct epoll_event timerEvent;
                                timerEvent.events = EPOLLIN;
                                timerEvent.data.fd = this->mHeartBeatTimerfd;
                                if (-1 ==
                                    epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->mHeartBeatTimerfd, &timerEvent)) {
                                    perror("SessionModule::didReceivedMessage()向Epoll实例注册定时器事件失败");
                                    exit(-1);
                                }
                            }
                        }

                        //2.调用回调函数
                        if (this->buildSessionSuccessCallback) {
                            this->buildSessionSuccessCallback(*this);
                            this->buildSessionSuccessCallback = nullptr;
                        }
                        this->state = SessionModuleState_SessionBuilded;
                    }else if(responseSessionIDMessage.status() == Node::ResponseSessionIDMessage_Status_ServerInterlnalError){
                        this->state = SessionModuleState_SessionFailed;
                        //2.调用回调函数
                        if (this->buildSessionFailureCallback) {
                            this->buildSessionFailureCallback(*this,BuildSessionFailureReasonType_ServerInteralError);
                            this->buildSessionFailureCallback = nullptr;
                        }
                    }
                }

            } else {
                //将消息插入到输入队列
                std::unique_ptr<::google::protobuf::Message> messageDuplicated(message.New());;
                messageDuplicated->ParseFromString(message.SerializeAsString());

                std::cout << "SessionModule::" << __FUNCTION__ << "接收到一条" << messageDuplicated->GetTypeName() << "消息"
                          << std::endl;

                this->mInputMessageQueue.emplace(std::move(messageDuplicated));
                u_int64_t messageCount = 1;
                if (sizeof(messageCount) != ::write(this->mInputMessageEventfd, &messageCount, sizeof(messageCount))) {
                    std::cout << "SessionModule::" << __func__ << __LINE__ << "发送messageEvent失败" << std::endl;
                }
            }
        }
    }

}
