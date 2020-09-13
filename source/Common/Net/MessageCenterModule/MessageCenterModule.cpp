//
// Created by taroyuyu on 2018/1/7.
//

#include "MessageCenterModule.h"
#include <functional>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>


namespace kakaIM {
    namespace net {
        //数据报首部长度
        const int kDatagramHeaderSize = 6;

        MessageCenterModule::MessageCenterModule(std::string listenIP, u_int16_t listenPort,
                                                 std::shared_ptr<MessageCenterAdapter> adapter, size_t timeout) :
                mAdapater(adapter), m_serverSocket(listenIP, listenPort, TCPServerSocket::On, TCPServerSocket::On),
                 mEpollInstance(-1), timmingWheelTimerfd(-1) {
            this->keepAlive = (timeout > 0);
            for (size_t i = 0; i < (timeout + 1); ++i) {
                this->connectionTimingWheel.emplace_back(std::set<int>());
            }
            this->nextExpirewheelPointer = this->connectionTimingWheel.begin();
            this->safeWheelPointer = this->connectionTimingWheel.end();
            this->safeWheelPointer--;
        }

        MessageCenterModule::~MessageCenterModule() {

        }

        bool MessageCenterModule::init() {
            if (nullptr == this->mAdapater) {
                return false;
            }
            //创建Epoll实例
            if (-1 == (this->mEpollInstance = epoll_create1(0))) {
                return false;
            }

            //timmingWheelTimerfd
            if (this->keepAlive) {
                this->timmingWheelTimerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);;
                if (this->timmingWheelTimerfd < 0) {
                    return false;
                }

                struct itimerspec timerConfig;
                timerConfig.it_value.tv_sec = 1;//1秒之后超时
                timerConfig.it_value.tv_nsec = 0;
                timerConfig.it_interval.tv_sec = 1;//间隔周期也是1秒
                timerConfig.it_interval.tv_nsec = 0;
                if (-1 == timerfd_settime(this->timmingWheelTimerfd, 0, &timerConfig, nullptr)) {//相对定时器
                    return false;
                }


                //向Epill实例注册timmingWheelTimerfd
                struct epoll_event timmingWheelTimerEvent;
                timmingWheelTimerEvent.events = EPOLLIN;
                timmingWheelTimerEvent.data.fd = this->timmingWheelTimerfd;
                if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->timmingWheelTimerfd,
                                    &timmingWheelTimerEvent)) {
                    return false;
                }
            }
            return true;
        }

        void MessageCenterModule::execute() {
            this->m_serverSocket.listen(50);

            struct epoll_event serverSocketEvent;
            serverSocketEvent.events = EPOLLIN | EPOLLERR;
            serverSocketEvent.data.fd = this->m_serverSocket.getSocketfd();

            //注册serverSocket的读事件和错误事件
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->m_serverSocket.getSocketfd(),
                                &serverSocketEvent)) {
                perror("MessageCenterModule::execute()注册serverSOcket的读事件和错误事件失败");
            }

            while (true == this->m_isStarted) {

                int const kHandleEventMaxCountPerLoop = 1024;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   1000);

                if (-1 == happedEventsCount) {
                    perror("MessageCenterModule::execute() 等待Epoll实例上的事件出错");
                }

                for (int i = 0; i < happedEventsCount; ++i) {
                    if (happedEvents[i].data.fd == this->m_serverSocket.getSocketfd()) {
                        if (EPOLLIN & happedEvents[i].events) {//有新的连接
                            //接收连接
                            int connectionFD = this->acceptConnection();
                            if (this->keepAlive) {
                                this->refreshConnection(connectionFD);
                            }
                        }

                        if (EPOLLERR & happedEvents[i].events) {//连接出错
                            std::cout << "MessageCenterModule::" << __FUNCTION__ << " serverSocket fd = "
                                      << happedEvents[i].data.fd << " 连接出错" << std::endl;
                        }
                    } else if (happedEvents[i].data.fd != this->timmingWheelTimerfd) {//处理客户端上的数据
                        if (EPOLLRDHUP & happedEvents[i].events) {//连接关闭或读半关闭
                            //尝试读取连接上的数据，并处理
                            if(EPOLLIN & happedEvents[i].events){
                                this->readConnectionData(happedEvents[i].data.fd);
                            }
                            //关闭连接
                            this->closeConnection(happedEvents[i].data.fd);
                            continue;//后续的写操作和储物处理不在处理
                        } else if (EPOLLIN & happedEvents[i].events) {//连接上有数据到达
                            if (this->keepAlive) {
                                //刷新此连接
                                this->refreshConnection(happedEvents[i].data.fd);
                            }
                            //读取连接上的数据，并处理
                            this->readConnectionData(happedEvents[i].data.fd);
                        }

                        if (EPOLLOUT & happedEvents[i].events) {//连接上有数据待发送，且此时可以发送数据

                            //尝试发送数据
                            if (false == tryTowriteConnection(happedEvents[i].data.fd)) {
                                //若连接上未有数据待发送，则让Epoll实例不再监听此连接的写事件
                                //让Epoll实例不再监听此连接的写事件
                                struct epoll_event connectionEvent;
                                connectionEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
                                connectionEvent.data.fd = happedEvents[i].data.fd;
                                if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_MOD, happedEvents[i].data.fd,
                                                    &connectionEvent)) {
                                    perror("MessageCenterModule::execute()取消监听此连接的写事件失败");
                                }
                            }
                        }
                        if (EPOLLERR & happedEvents[i].events) {//连接出错
                            std::cout<<"MessageCenterModule::"<<__FUNCTION__<< " fd = " << happedEvents[i].data.fd << " 连接出错" << std::endl;
                            //关闭连接
                            this->closeConnection(happedEvents[i].data.fd);
                        }
                    } else {//处理TimingWheel
                        uint64_t expireCount = 0;
                        if (sizeof(uint64_t) != ::read(this->timmingWheelTimerfd, &expireCount, sizeof(expireCount))) {
                            std::cout << "MessageCenterModule::" << __FUNCTION__ << "read(timmingWheelTimerfd)出错"
                                      << std::endl;
                            return;
                        }

                        if (this->keepAlive) {
                            while (expireCount--) {
                                this->refreshTimingWheel();
                            }
                        }
                    }
                }


            }
        }

        std::string MessageCenterModule::generateConnectionIdentifier(int socketfd) {
            timeval tv;
            gettimeofday(&tv, 0);
            return std::to_string((uint64_t) tv.tv_sec * 1000 + (uint64_t) tv.tv_usec / 1000) +
                   std::to_string(socketfd);
        }

        int MessageCenterModule::acceptConnection() {
            //1.接收连接
            TCPClientSocket clientSocket = this->m_serverSocket.accept();
            clientSocket.setRecvBufferLowait(kDatagramHeaderSize);
            //2.分配缓冲区
            this->socketInputBufferMap.erase(clientSocket.getSocketfd());
            this->socketInputBufferMap.emplace(
                    clientSocket.getSocketfd(), std::unique_ptr<CircleBuffer>(new CircleBuffer(
                            kDatagramHeaderSize)));
            this->socketOutputBufferMap.erase(clientSocket.getSocketfd());
            this->socketOutputBufferMap.emplace(
                    clientSocket.getSocketfd(), std::unique_ptr<CircleBuffer>(new CircleBuffer(
                            kDatagramHeaderSize)));
            //3.为此连接生成connectionIdentifier,并将其加入连接池
            const std::string connectionIdentifier = this->generateConnectionIdentifier(clientSocket.getSocketfd());
            this->mConnectionPool.erase(clientSocket.getSocketfd());
            this->mConnectionPool.emplace(clientSocket.getSocketfd(), connectionIdentifier);
            //4.向Epoll实例中注册此连接的读事件、连接关闭事件、连接出错事件
            struct epoll_event connectionEvent;
            connectionEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
            connectionEvent.data.fd = clientSocket.getSocketfd();
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, clientSocket.getSocketfd(), &connectionEvent)) {
                perror("MessageCenterModule::acceptConnection()注册新连接的读事件和错误事件失败");
            }
            //5.若开启KeepAlive机制，则将其加入当前时间lun
            if (this->keepAlive) {
                this->refreshConnection(clientSocket.getSocketfd());
            }

            return clientSocket.getSocketfd();
        }

        void MessageCenterModule::closeConnection(uint32_t connectionFD) {
            //1.关闭连接
            ::close(connectionFD);
            this->connectionRetainCountTable.erase(connectionFD);
            //2.释放缓冲区
            this->socketInputBufferMap.erase(connectionFD);
            this->socketOutputBufferMap.erase(connectionFD);
            //3.从Epoll实例中移除此连接的相关事件
            epoll_ctl(this->mEpollInstance, EPOLL_CTL_DEL, connectionFD, nullptr);

            auto connectionIdentifierIt = this->mConnectionPool.find(connectionFD);
            if (connectionIdentifierIt != this->mConnectionPool.end()) {
                const std::string expireConnectionIdentifier = connectionIdentifierIt->second;
                //4.从连接池中清除对应的连接
                this->mConnectionPool.erase(connectionIdentifierIt);
                //5.通知ConnectionDelegate
                for (auto connectionDelegate : this->connectionDelegateList) {
                    connectionDelegate->didCloseConnection(expireConnectionIdentifier);
                }
            }
        }

        void MessageCenterModule::readConnectionData(uint32_t connectionFD) {
            //1.将数据读入输入缓冲区
            static u_int8_t buffer[1024] = {0};
            ssize_t readCount = read(connectionFD, buffer, 1024);
            if (readCount > 0) {
                auto it = this->socketInputBufferMap.find(connectionFD);
                if (it == this->socketInputBufferMap.end()) {
                    //错误处理
                    std::cout << "MessageCenterModule::" << __FUNCTION__ << "异常" << __LINE__ << std::endl;
                    //连接关闭
                    this->closeConnection(connectionFD);
                } else {
                    it->second->append(buffer, readCount);
                    ::google::protobuf::Message *message = nullptr;
                    while (this->mAdapater->tryToretriveMessage(*it->second, &message)) {
                        //分发消息
                        this->dispatchMessage(connectionFD, *message);
                        delete message;
                    };

                    //std::cout<<<<"输入缓冲区中已经不存在一条完整的消息"<<std::endl;
                }
            } else if (EOF == readCount) {
                //连接关闭
                this->closeConnection(connectionFD);
            } else {
                //连接出错
                this->closeConnection(connectionFD);
            }
        }

        bool MessageCenterModule::tryTowriteConnection(u_int32_t connectionFD) {
            auto outputBufferIt = this->socketOutputBufferMap.find(connectionFD);
            if (outputBufferIt == this->socketOutputBufferMap.end()) {
                //错处处理
                std::cout << "fd = " << connectionFD << "有数据待发送，但是其相关的输出缓冲区找不到" << std::endl;
                return false;
            } else {
                //1.判断缓冲区中是否还待发送的数据
                if (outputBufferIt->second->getUsed() > 0) {//缓冲区中存在待发送的数据
                    //1.获得socket发送缓冲区的容量
                    size_t output_buf_size;
                    socklen_t output_buf_size_len = sizeof(output_buf_size);
                    if (getsockopt(connectionFD, SOL_SOCKET, SO_SNDBUF, (void *) &output_buf_size,
                                   &output_buf_size_len) == -1) {
                        //错误处理
                    }
                    //2.获得socket发送缓存区已使用的容量
                    size_t outputBufUsed;
                    if (EINVAL == ioctl(connectionFD, TIOCOUTQ, &outputBufUsed)) {
                        //错误处理
                    }
                    //3.计算socket发送缓冲区的剩余容量
                    size_t output_free_size = output_buf_size - outputBufUsed;
                    //4.判断此时发送所需的内存空间
                    const size_t writeBytes = std::min(output_free_size,outputBufferIt->second->getUsed());
                    static u_int8_t *buffer = nullptr;
                    static size_t currentBufferSize = 0;
                    //5.更新缓冲区: 若本次write操作所能写入的最大字节数大于buffer,则删除旧的缓冲区，并分配新的缓冲区
                    if (writeBytes > currentBufferSize) {
                        delete[] buffer;//一定要使用delete[] 进行删除，而不是delete,因为buffer本质上是指向一个数组
                        buffer = new u_int8_t[writeBytes];
                        currentBufferSize = writeBytes;
                    }
                    //6.从缓冲区中读取数据
                    size_t retrivedCount = outputBufferIt->second->retrive(buffer, writeBytes);
                    //7.将数据写入socket的发送缓冲区
                    ssize_t writeCount = write(connectionFD, buffer, retrivedCount);
                    if (retrivedCount != writeCount) {
                        if(-1 == writeCount){
                            std::cerr << __FUNCTION__ << __LINE__ << "执行往socket输出缓冲区中输出数据时出现差错,errno="<<errno<< std::endl;
                        }else{
                            size_t leftCount = retrivedCount - writeBytes;
                            while (leftCount){
                                ssize_t count = write(connectionFD,buffer + writeCount,leftCount);
                                if(-1 != count){
                                    leftCount-=count;
                                    writeCount+=count;
                                }else{
                                    //错误处理
                                    std::cerr << __FUNCTION__ << __LINE__ << "执行往socket输出缓冲区中输出数据时出现差错" << std::endl;
                                    break;
                                }
                            }
                        }
                    }
                    return true;
                } else {
                    return false;
                }
            }
        }

        void MessageCenterModule::dispatchMessage(int socketfd, ::google::protobuf::Message &message) {
            auto handlerIt = this->messageHandler.find(message.GetTypeName());
            if (this->messageHandler.end() != handlerIt) {
                //1.从连接池中获得连接
                auto connectionIdentifierIt = this->mConnectionPool.find(socketfd);
                std::string connectionIdentifier;
                if (connectionIdentifierIt == this->mConnectionPool.end()) {//不存在
                    //异常处理
                    std::cerr << "MessageCenterModule::" << __FUNCTION__ << " 找不到socketfd=" << socketfd
                              << " 对应的连接标志符connectionIdentifier" << std::endl;
                    return;;
                }

                connectionIdentifier = connectionIdentifierIt->second;
                if (false == this->messageFilterList.empty()) {
                    auto it = this->messageFilterList.begin();
                    bool flag = true;
                    while (it != this->messageFilterList.end()) {
                        if ((*it)->doFilter(message, connectionIdentifier)) {
                            //消息通过，执行下一个过滤器
                            it++;
                            continue;
                        } else {
                            flag = false;
                            break;
                        }
                    }
                    if (flag) {
                        handlerIt->second(message, connectionIdentifier);
                    }
                } else {
                    handlerIt->second(message, connectionIdentifier);
                }
            }
        }

        void MessageCenterModule::sendMessage(int socketfd, const ::google::protobuf::Message &message) {
            //1.找到socket对象的输出缓冲区
            auto outputBufferIt = this->socketOutputBufferMap.find(socketfd);
            if (outputBufferIt == this->socketOutputBufferMap.end()) {
                //错误处理
                std::cout << "MessageCenterModule::" << __FUNCTION__ << "异常" << __LINE__ << std::endl;
            } else {
                //2.由适配器对消息进行封装并将消息放入输出缓冲区
                this->mAdapater->encapsulateMessageToByteStream(message, *outputBufferIt->second);
                //3.让Epoll实例监听此连接的Write事件
                struct epoll_event connectionEvent;
                connectionEvent.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR;
                connectionEvent.data.fd = socketfd;
                if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_MOD, socketfd, &connectionEvent)) {
                    perror("MessageCenterModule::sendMessage()监听此连接的写事件失败");
                }
            }
        }

        void MessageCenterModule::setMessageHandler(const std::string &messageType,
                                                    std::function<void(const ::google::protobuf::Message &,
                                                                       std::string)> messageHandler) {
            this->messageHandler[messageType] = messageHandler;
        }

        void MessageCenterModule::addMessageFilster(MessageFilter *filter) {
            auto filterIt = std::find(this->messageFilterList.begin(), this->messageFilterList.end(), filter);
            if (filterIt == this->messageFilterList.end()) {
                this->messageFilterList.emplace_back(filter);
            }
        }

        void MessageCenterModule::addConnectionDelegate(ConnectionDelegate *delegate) {
            auto delegateIt = std::find(connectionDelegateList.begin(), connectionDelegateList.end(), delegate);
            if (delegateIt == this->connectionDelegateList.end()) {
                this->connectionDelegateList.emplace_back(delegate);
            }
        }

        bool
        MessageCenterModule::sendMessage(std::string connectionIdentifier, const ::google::protobuf::Message &message) {
            auto it = std::find_if(this->mConnectionPool.begin(), this->mConnectionPool.end(), [connectionIdentifier](
                    const std::map<int, const std::string>::value_type item) -> bool {
                return item.second == connectionIdentifier;
            });

            if (it != this->mConnectionPool.end()) {
                this->sendMessage(it->first, message);
                return true;
            } else {
                return false;
            }
        }

        std::pair<bool, std::pair<std::string, uint16_t>>
        MessageCenterModule::queryConnectionAddress(const std::string connectionIdentifier) {
            auto it = std::find_if(this->mConnectionPool.begin(), this->mConnectionPool.end(), [connectionIdentifier](
                    const std::map<int, const std::string>::value_type item) -> bool {
                return item.second == connectionIdentifier;
            });


            if (it != this->mConnectionPool.end()) {
                TCPClientSocket clientSocket = it->first;
                struct sockaddr_in remoteAddr;
                memset(&remoteAddr, 0, sizeof(remoteAddr));
                socklen_t remoteAddr_len = sizeof(remoteAddr);
                if (-1 == ::getpeername(it->first, (struct sockaddr *) &remoteAddr, &remoteAddr_len)) {
                    return std::make_pair(false, std::make_pair("", 0));
                }


                if (remoteAddr_len != sizeof(remoteAddr)) {
                    return std::make_pair(false, std::make_pair("", 0));
                }

                return std::make_pair(true,
                                      std::make_pair(::inet_ntoa(remoteAddr.sin_addr), ::ntohs(remoteAddr.sin_port)));
            } else {
                return std::make_pair(false, std::make_pair("", 0));
            }
        }

        uint64_t MessageCenterModule::currentConnectionCount() {
            return this->mConnectionPool.size();
        }

        void MessageCenterModule::refreshConnection(int connection) {
            std::lock_guard<std::mutex> lock(this->timmingWheelMutex);
            assert(this->safeWheelPointer != this->connectionTimingWheel.end());
            auto recordIt = (*this->safeWheelPointer).find(connection);
            if(recordIt != this->safeWheelPointer->end()){
                return;
            }
            this->safeWheelPointer->emplace(connection);
            auto connectionRetainRecordIt = this->connectionRetainCountTable.find(connection);
            if (this->connectionRetainCountTable.end() != connectionRetainRecordIt) {
                this->connectionRetainCountTable[connection] = connectionRetainRecordIt->second+1;
            } else {
                this->connectionRetainCountTable.emplace(connection, 1);
            }
        }

        void MessageCenterModule::refreshTimingWheel() {
            std::lock_guard<std::mutex> lock(this->timmingWheelMutex);

            //时间轮滚动
            this->nextExpirewheelPointer++;
            this->safeWheelPointer++;

            if (this->connectionTimingWheel.end() == this->nextExpirewheelPointer) {
                this->nextExpirewheelPointer = this->connectionTimingWheel.begin();
            }

            if (this->connectionTimingWheel.end() == this->safeWheelPointer) {
                this->safeWheelPointer = this->connectionTimingWheel.begin();
            }

            //减少此时间轮上的连接的引用计数
            for (auto connectionFd : *this->nextExpirewheelPointer) {
                //1.从连接池中获得连接
                auto connectionIt = this->mConnectionPool.find(connectionFd);
                if (connectionIt != this->mConnectionPool.end()) {
                    auto connectionRetainCountIt = this->connectionRetainCountTable.find(connectionFd);
                    if (connectionRetainCountIt != this->connectionRetainCountTable.end()) {


                        if (1 < connectionRetainCountIt->second) {
                            //将此连接的引用计数减少
                            this->connectionRetainCountTable[connectionFd] = connectionRetainCountIt->second -1;
                        } else {//关闭连接
                            this->closeConnection(connectionFd);
                        }
                    } else {//不存在关于此连接的引用计数，则直接关闭此连接
                        this->closeConnection(connectionFd);
                    }
                }
            }
            //清空此时间轮
            this->nextExpirewheelPointer->clear();
        }
    }
}
