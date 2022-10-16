//
// Created by Kakawater on 2018/1/29.
//

#ifndef KAKAIMCLUSTER_SESSIONMODULE_H
#define KAKAIMCLUSTER_SESSIONMODULE_H

#include <string>
#include <functional>
#include <queue>
#include <memory>
#include <Common/KIMModule.h>
#include <Common/Net/TCPSocket/TCPClientSocket.h>
#include <Common/Net/TCPSocketManager/TCPSocketManager.h>
#include <Common/Net/TCPSocketManager/TCPSocketManagerConsignor.h>
#include <Common/KakaIMMessageAdapter.h>
#include <Client/ServiceModule/ServiceModule.h>

namespace kakaIM {
    namespace client {
        class ServiceModule;

        class SessionModule : public common::KIMModule, public net::TCPSocketManagerConsignor {
            friend class ServiceModule;

        public:
            virtual bool init() override;

            typedef std::function<void(SessionModule &)> BuildSessionSuccessCallback;
            enum BuildSessionFailureReasonType {
                BuildSessionFailureReasonType_ServerInteralError,//服务器内部错误
            };
            typedef std::function<void(SessionModule &,BuildSessionFailureReasonType failureReasonType)> BuildSessionFailureCallback;

            SessionModule(std::string serverAddr, uint32_t serverPort);

            ~SessionModule();

            void addServiceModule(ServiceModule *serviceModule);

            void buildSession(BuildSessionSuccessCallback success, BuildSessionFailureCallback failure);

            void destorySession();

            virtual void didConnectionClosed(net::TCPClientSocket clientSocket) override;

            virtual void didReceivedMessage(int socketfd, const ::google::protobuf::Message &) override;

            /**
             * 发送消息
             * @param message
             */
            void sendMessage(const google::protobuf::Message &message);

        protected:

            virtual void execute() override;

        private:
            int mEpollInstance;

            //消息输入队列，用于从存放socketManager接收到的消息
            int mInputMessageEventfd;
            std::queue<std::unique_ptr<::google::protobuf::Message>> mInputMessageQueue;

            void handleInputMessage(::google::protobuf::Message &message);

            //消息输出队列，用于存放待发送的消息
            int mOutputMessageEventfd;
            std::queue<std::unique_ptr<::google::protobuf::Message>> mOutputMessageQueue;

            void handleOutputMessage(::google::protobuf::Message &message);

            std::unique_ptr<net::TCPSocketManager> mSocketManager;

            common::KakaIMMessageAdapter *mKakaIMMessageAdapter;

            std::string serverAddr;
            uint32_t serverPort;
            net::TCPClientSocket mConnectionSocket;

            enum SessionModuleState {
                SessionModuleState_UNConnect,//未连接
                SessionModuleState_Connected,//已连接
                SessionModuleState_SessionBuilding,//正在构建会话
                SessionModuleState_SessionBuilded,//会话构建完毕
                SessionModuleState_SessionFailed,//会话构建失败
            } state;
            int mHeartBeatTimerfd;//心跳定时器

            std::string currentSessionID;

            std::map<std::string, std::list<ServiceModule *>> moduleList;

            BuildSessionSuccessCallback buildSessionSuccessCallback;
            BuildSessionFailureCallback buildSessionFailureCallback;
        };
    }
}


#endif //KAKAIMCLUSTER_SESSIONMODULE_H
