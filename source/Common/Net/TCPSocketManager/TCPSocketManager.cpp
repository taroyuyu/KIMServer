//
// Created by taroyuyu on 2018/1/10.
//

#include "TCPSocketManager.h"
#include "TCPSocketManagerConsignor.h"
#include "TCPSocketManaerAdapter.h"
#include "../Buffer/CircleBuffer.h"
#include <sys/select.h>
#include <zconf.h>
#include <iostream>
#include <cstdio>
#include <netinet/in.h>
#include <sys/epoll.h>

namespace kakaIM {
    namespace net {
        //数据报首部长度
        const int kDatagramHeaderSize = 6;

        void TCPSocketManager::execute() {
            while (true == this->m_isStarted) {
                int const kHandleEventMaxCountPerLoop = 1024;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   10000);

                if (-1 == happedEventsCount) {
                    perror("TCPSocketManager::execute() 等待Epoll实例上的事件出错");
                }

                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {

                    if (EPOLLRDHUP & happedEvents[i].events) {//连接关闭或读半关闭
                        std::cout << "fd = " << happedEvents[i].data.fd << "对方连接关闭" << std::endl;
                        //关闭连接
                        this->closeConnection(happedEvents[i].data.fd);
                    } else if (EPOLLIN & happedEvents[i].events) {//连接上有数据到达
                        //1.将数据读入输入缓冲区
                        TCPClientSocket clientSocket = happedEvents[i].data.fd;
                        static u_int8_t buffer[1024] = {0};

                        auto it = this->mSocketInputBufferMap.find(happedEvents[i].data.fd);
                        if (it == this->mSocketInputBufferMap.end()) {
                            std::cout << "哎呦，竟然没有为" << happedEvents[i].data.fd << "分配输入缓冲区" << std::endl;
                            //错误处理
                        } else {

                            ssize_t readCount = 0;

                            //读取连接上的数据，并处理
                            while (-1 != readCount) {
                                readCount = ::recv(happedEvents[i].data.fd, buffer, sizeof(buffer), MSG_DONTWAIT);
                                if (-1 == readCount) {
                                    //if (false == (::EAGIN == ::errno || ::EINTR == ::errno)){//有错误

                                    //}
                                    //
                                } else {
                                    //将字节流放入缓冲区
                                    it->second->append(buffer, readCount);
                                }
                            }

                            auto connectionIt = this->mConnectionConsignorTable.find(happedEvents[i].data.fd);
                            if (connectionIt == this->mConnectionConsignorTable.end()) {
                                return;
                            }

                            auto connectionAdapter = connectionIt->second.second;

                            ::google::protobuf::Message *message = nullptr;
                            while (connectionAdapter->tryToretriveMessage(*it->second, &message)) {
                                //分发消息
                                this->dispatchMessage(happedEvents[i].data.fd, *message);
                                delete message;
                            }
                        }
                        /*readCount = ::read(happedEvents[i].data.fd, buffer, sizeof(buffer));

                        if(readCount > 0){
                            auto it = this->mSocketInputBufferMap.find(happedEvents[i].data.fd);
                            if (it == this->mSocketInputBufferMap.end()) {
                                std::cout << "哎呦，竟然没有为" << happedEvents[i].data.fd << "分配输入缓冲区" << std::endl;
                                //错误处理
                            } else {
                                //将字节流放入缓冲区
                                it->second->append(buffer, readCount);
                                auto connectionIt = this->mConnectionConsignorTable.find(happedEvents[i].data.fd);
                                if(connectionIt == this->mConnectionConsignorTable.end()){
                                    return;
                                }
                                auto connectionAdapter = connectionIt->second.second;

                                ::google::protobuf::Message *message = nullptr;
                                if (connectionAdapter->tryToretriveMessage(it->second, &message)) {
                                    //分发消息
                                    this->dispatchMessage(happedEvents[i].data.fd, *message);
                                    delete message;
                                }
                            }
                        }*/
                    }

                    if (EPOLLOUT & happedEvents[i].events) {//连接上有数据待发送，且此时可以发送数据
                        auto it = this->mSocketOutputBufferMap.find(happedEvents[i].data.fd);
                        if (it == this->mSocketOutputBufferMap.end()) {
                            //错处处理
                            //让Epoll实例不再监听此连接的写事件
                            struct epoll_event connectionEvent;
                            connectionEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
                            connectionEvent.data.fd = happedEvents[i].data.fd;
                            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_MOD, happedEvents[i].data.fd,
                                                &connectionEvent)) {
                                perror("TCPSocketManager::execute()取消监听此连接的写事件失败");
                            }
                        } else {
                            //1.判断缓冲区中是否还待发送的数据
                            if (it->second->getUsed() > 0) {//缓冲区中存在待发送的数据
                                //1.获得socket发送缓冲区的容量
                                size_t socketOutputBuffer_Capacity;
                                socklen_t output_buf_size_len = sizeof(socketOutputBuffer_Capacity);
                                if (getsockopt(happedEvents[i].data.fd, SOL_SOCKET, SO_SNDBUF,
                                               (void *) &socketOutputBuffer_Capacity,
                                               &output_buf_size_len) == -1) {
                                    //错误处理
                                }
                                //2.获得socket发送缓存区已使用的容量
                                size_t socketOutputBuffer_Used;
                                if (EINVAL == ioctl(happedEvents[i].data.fd, TIOCOUTQ, &socketOutputBuffer_Used)) {
                                    //错误处理
                                }
                                //3.计算socket发送缓冲区的剩余容量
                                size_t socketOutputBuffer_Free = socketOutputBuffer_Capacity - socketOutputBuffer_Used;

                                //4.计算本次write操作所能写入的最大字节数
                                const size_t writeBytes =
                                        socketOutputBuffer_Free < it->second->getUsed() ? socketOutputBuffer_Free
                                                                                        : it->second->getUsed();
                                static u_int8_t *buffer = nullptr;// = new u_int8_t[output_free_size < it->second->getUsed() ? output_free_size : it->second->getUsed()];
                                static size_t currentBufferSize = 0;


                                //5.更新缓冲区: 若本次write操作所能写入的最大字节数大于buffer,则删除旧的缓冲区，并分配新的缓冲区
                                if (writeBytes > currentBufferSize) {
                                    delete[] buffer;//一定要使用delete[],因为buffer本质上是指向一个数组
                                    buffer = new u_int8_t[writeBytes];
                                    currentBufferSize = writeBytes;
                                }
                                //6.从输出缓冲区中获取数据，数目最多为socket发送缓冲区的空闲空间
                                const int retrivedCount = it->second->retrive(buffer, writeBytes);
                                //7.写入socket的发送缓冲区
                                if (retrivedCount != write(happedEvents[i].data.fd, buffer, retrivedCount)) {
                                    //错误处理
                                    std::cout << "TCPSocketManager::" << __FUNCTION__ << "发送出错" << std::endl;
                                }
                            } else {//缓冲区中不存在待发送的数据
                                //取消监听socket的发送缓冲区的事件
                                //让Epoll实例不再监听此连接的写事件
                                struct epoll_event connectionEvent;
                                connectionEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
                                connectionEvent.data.fd = happedEvents[i].data.fd;
                                if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_MOD, happedEvents[i].data.fd,
                                                    &connectionEvent)) {
                                    perror("TCPSocketManager::execute()取消监听此连接的写事件失败");
                                }

                            }
                        }
                    }
                }
            }
        }


        void TCPSocketManager::dispatchMessage(int socketfd, const ::google::protobuf::Message &message) {
            //发送给相应的ConnectionDelegate
            auto consignotIt = this->mConnectionConsignorTable.find(socketfd);
            if (consignotIt != this->mConnectionConsignorTable.end()) {
                consignotIt->second.first->didReceivedMessage(socketfd, message);
            }
        }

        void TCPSocketManager::closeConnection(int socketfd) {
            //1.关闭连接
            close(socketfd);
            //2.清除关联的输入缓冲区和输出缓冲区
            this->mSocketInputBufferMap.erase(socketfd);

            this->mSocketOutputBufferMap.erase(socketfd);

            //3.从Epoll实例中移除此连接的相关事件
            epoll_ctl(this->mEpollInstance, EPOLL_CTL_DEL, socketfd, nullptr);

            //3.通知ConnectionDelegate
            auto consignotIt = this->mConnectionConsignorTable.find(socketfd);
            if (consignotIt != this->mConnectionConsignorTable.end()) {
                consignotIt->second.first->didConnectionClosed(socketfd);
            }

            this->mConnectionConsignorTable.erase(consignotIt);

            //4.从mSocketTable中移除
            auto fdIt = std::find(this->mSocketTable.begin(), this->mSocketTable.end(), socketfd);
            this->mSocketTable.erase(fdIt);
        }

        TCPSocketManager::TCPSocketManager() :
                m_isStarted(false) {
            //创建Epoll实例
            if (-1 == (this->mEpollInstance = epoll_create1(0))) {
                std::cout << "TCPSocketManager初始化失败" << std::endl;
                //抛出异常
            }
        }

        TCPSocketManager::~TCPSocketManager() {
            this->stop();
        }

        void TCPSocketManager::start() {
            if (false == this->m_isStarted) {
                this->m_isStarted = true;
                this->m_workThread = std::move(std::thread([this]() {
                    this->execute();
                    this->m_isStarted = false;
                }));
            }
        }

        void TCPSocketManager::stop() {
            if (true == this->m_isStarted) {
                this->m_isStarted = false;
            }
        }

        void TCPSocketManager::addSocket(int socketfd, TCPSocketManagerConsignor *consignor,
                                         TCPSocketManaerAdapter *adapter) {
            if (consignor == nullptr || adapter == nullptr) {
                std::cout << "TCPSocketManager::" << __FUNCTION__ << "consignor或adapter为nullptr" << std::endl;
                return;
            }
            this->mSocketTable.emplace_back(socketfd);
            this->mConnectionConsignorTable[socketfd] = std::make_pair(consignor, adapter);
            // 分配缓冲区
            this->mSocketInputBufferMap.erase(socketfd);
            this->mSocketOutputBufferMap.erase(socketfd);
            this->mSocketInputBufferMap.emplace(
                    socketfd, std::unique_ptr<CircleBuffer>(new CircleBuffer(512)));
            this->mSocketOutputBufferMap.emplace(
                    socketfd, std::unique_ptr<CircleBuffer>(new CircleBuffer(512)));
            //
            //向Epoll实例中注册此连接的读事件、连接关闭事件、连接出错事件
            struct epoll_event connectionEvent;
            connectionEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
            connectionEvent.data.fd = socketfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, socketfd, &connectionEvent)) {
                perror("TCPSocketManager::addSocket()注册新连接的读事件和错误事件失败");
            }
        }

        void TCPSocketManager::addSocket(TCPClientSocket &socket, TCPSocketManagerConsignor *consignor,
                                         TCPSocketManaerAdapter *adapter) {
            this->addSocket(socket.getSocketfd(), consignor, adapter);
        }

        void TCPSocketManager::sendMessage(TCPClientSocket &socket, const ::google::protobuf::Message &message) {
            this->sendMessage(socket.getSocketfd(), message);
        }

        void
        TCPSocketManager::sendMessage(int socketfd, const ::google::protobuf::Message &message) {
            //根据socket寻找发送缓冲区
            auto it = this->mSocketOutputBufferMap.find(socketfd);
            if (it == this->mSocketOutputBufferMap.end()) {
                //抛出异常
                std::cout << __FUNCTION__ << "异常" << std::endl;
                return;
            }

            auto connectionIt = this->mConnectionConsignorTable.find(socketfd);
            if (connectionIt == this->mConnectionConsignorTable.end()) {
                //抛出异常
                std::cout << __FUNCTION__ << "异常" << std::endl;
                return;
            }

            //2.由适配器对消息进行封装并将消息放入输出缓冲区
            auto connectionAdapter = connectionIt->second.second;
            connectionAdapter->encapsulateMessageToByteStream(message, *it->second);
            //3.让Epoll实例监听此连接的Write事件
            struct epoll_event connectionEvent;
            connectionEvent.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLERR;
            connectionEvent.data.fd = socketfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_MOD, socketfd, &connectionEvent)) {
                perror("TCPSocketManager::sendMessage()监听此连接的写事件失败");
            }
        }
    }
}
