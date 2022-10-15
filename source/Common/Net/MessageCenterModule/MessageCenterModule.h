//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_MESSAGECENTERMODULE_H
#define KAKAIMCLUSTER_MESSAGECENTERMODULE_H

#include <google/protobuf/message.h>
#include <string>
#include <map>
#include <list>
#include <mutex>
#include <memory>
#include <functional>
#include "../../KIMModule.h"
#include "../Buffer/CircleBuffer.h"
#include "../TCPSocket/TCPServerSocket.h"
#include "MessageCenterAdapter.h"
#include "MessageFilter.h"
#include "ConnectionDelegate.h"

namespace kakaIM {
    namespace net {
        class MessageCenterModule
                : public kakaIM::common::KIMModule {
        public:
            MessageCenterModule(std::string listenIP, u_int16_t listenPort,
                                std::shared_ptr<MessageCenterAdapter> adapter,
                                size_t timeout = 0);

            ~MessageCenterModule();

            virtual bool init() override;

            void setMessageHandler(const std::string &messageType,
                                   std::function<void(std::unique_ptr<::google::protobuf::Message>,
                                                      const std::string)> messageHandler);

            void addMessageFilster(MessageFilter *filter);

            void addConnectionDelegate(ConnectionDelegate *delegate);

            bool sendMessage(std::string connectionIdentifier, const ::google::protobuf::Message &message);

            std::pair<bool, std::pair<std::string, uint16_t>>
            queryConnectionAddress(const std::string connectionIdentifier);

            uint64_t currentConnectionCount();

        protected:
            virtual void execute() override;

        private:
            std::shared_ptr<MessageCenterAdapter> mAdapater;
            std::list<MessageFilter *> messageFilterList;
            std::list<ConnectionDelegate *> connectionDelegateList;
            std::map<const std::string, std::function<void(std::unique_ptr<::google::protobuf::Message>,
                                                           const std::string)>> messageHandler;
            net::TCPServerSocket m_serverSocket;
            int mEpollInstance;
            /**
              * socket输入缓冲区
              */
            std::map<int, std::unique_ptr<net::CircleBuffer>> socketInputBufferMap;
            /**
             * socket输出缓冲区
             */
            std::map<int, std::unique_ptr<net::CircleBuffer>> socketOutputBufferMap;
            /**
             * 连接池,Key-value形式为socketfd-connectionIdentifier
             */
            std::map<int, const std::string> mConnectionPool;

            std::string generateConnectionIdentifier(int socketfd);

            int acceptConnection();

            void closeConnection(uint32_t connectionFD);

            void readConnectionData(uint32_t connectionFD);

            void dispatchMessage(int socketfd,std::unique_ptr<::google::protobuf::Message> message);

            void sendMessage(int socketfd, const ::google::protobuf::Message &message);

            bool tryTowriteConnection(u_int32_t connectionFD);

            /**
             * 时间轮
             */
            bool keepAlive;
            int timmingWheelTimerfd;
            std::mutex timmingWheelMutex;
            std::list<std::set<int>> connectionTimingWheel;
            std::list<std::set<int>>::iterator nextExpirewheelPointer;
            std::list<std::set<int>>::iterator safeWheelPointer;
            std::map<int, int> connectionRetainCountTable;

            void refreshConnection(int connection);

            void refreshTimingWheel();
        };
    }
}

#endif //KAKAIMCLUSTER_MESSAGECENTERMODULE_H
