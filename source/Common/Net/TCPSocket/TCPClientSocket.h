//
// Created by taroyuyu on 2018/1/3.
//

#ifndef KAKAIMCLUSTER_TCPCLIENTSOCKET_H
#define KAKAIMCLUSTER_TCPCLIENTSOCKET_H

#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <cerrno>
#include "TCPSocket.h"
#include <sys/socket.h>

namespace kakaIM {
    namespace net {
        class TCPClientSocket : public TCPSocket {
        public:
            TCPClientSocket(const std::string &localAddr = "", const int localPort = -1);

            TCPClientSocket(int socketfd);

            TCPClientSocket(const TCPClientSocket &clientSocket);

            virtual ~TCPClientSocket() = default;

            bool operator==(const TCPClientSocket &socket);

            void connect(const std::string serverAddr, const int serverPort,
                         SocketOptionState tcpNoDelay = SocketOptionState::On);

            /**
             * 发送数据
             * @param buffer 待发送的数据
             * @param bufferLength 数据长度
             * @return 返回已经发送的字节数
             */
            int send(char *buffer, long bufferLength);

            /**
             * 读取数据
             * @param buffer 接收数据的缓冲区
             * @param bufferLength 缓冲区的长度，即缓冲区的容量
             * @return 返回已经读取的字节数
             */
//            int read(char * buffer, long bufferLength);

            /**
             * 关闭写
             */
            void shutdownWrite();

            void close();

            /**
             * 获取接收缓冲区的大小
             * @return
             */
            size_t getRecvBufferSize() {
                size_t inputput_buf_size;
                socklen_t input_buf_size_len = sizeof(inputput_buf_size);
                if (getsockopt(this->m_socketfd, SOL_SOCKET, SO_RCVBUF, (void *) &inputput_buf_size,
                               &input_buf_size_len) == -1) {
                    //错误处理
                }
                return inputput_buf_size;
            }

            /**
             * 获取接收缓冲区中已使用的大小，即剩余未读取的数据字节数
             * @return
             */
            size_t getRecvBufferUsedSpaceSize() {
                size_t inputputBufUsed;
                if (EINVAL == ioctl(this->m_socketfd, SIOCINQ, &inputputBufUsed)) {
                    //错误处理
                }
                return inputputBufUsed;
            }

            /**
             * 获得发送缓冲区的大小
             * @return
             */
            size_t getSendBufferSize() {
                size_t output_buf_size;
                socklen_t output_buf_size_len = sizeof(output_buf_size);
                if (getsockopt(this->m_socketfd, SOL_SOCKET, SO_SNDBUF, (void *) &output_buf_size,
                               &output_buf_size_len) == -1) {
                    //错误处理
                }
                return output_buf_size;
            }

            /**
             * 获得发送缓冲区已使用的大小，即剩余未发送的数据字节数
             * @return
             */

            size_t getSendBufferUsedSpaceSize() {
                size_t outputBufUsed;
                if (EINVAL == ioctl(this->m_socketfd, SIOCOUTQ, &outputBufUsed)) {
                    //错误处理
                }
                return outputBufUsed;
            }

            /**
             * 获得发送区中未使用的大小，即还可以容纳的数据字节数
             * @return
             */
            size_t getSendBufferAvailableSpaceSize() {
                return this->getSendBufferSize() - this->getSendBufferAvailableSpaceSize();
            }

            /**
             * 设置接收缓冲区的低水位标记
             * @param lowait
             */
            void setRecvBufferLowait(size_t lowait) {
                setsockopt(this->m_socketfd, SOL_SOCKET, SO_RCVBUF, &lowait, sizeof(lowait));
            }

            /**
             * 设置发送缓冲区的低水位标记
             * @param lowait
             */
            void setSendBufferLowait(size_t lowait) {
                setsockopt(this->m_socketfd, SOL_SOCKET, SO_SNDLOWAT, &lowait, sizeof(lowait));
            }
        };
    }
}

#endif //KAKAIMCLUSTER_TCPCLIENTSOCKET_H
