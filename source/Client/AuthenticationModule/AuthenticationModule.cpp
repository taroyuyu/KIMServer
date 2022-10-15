//
// Created by Kakawater on 2018/2/1.
//

#include "AuthenticationModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"

namespace kakaIM {
    namespace client {
        AuthenticationModule::AuthenticationModule():sessionModule(nullptr) {

        }

        AuthenticationModule::~AuthenticationModule() {

        }

        void AuthenticationModule::registerAccount(std::string userAccount, std::string userPassword,
                                                   RegisterSuccessCallback success,
                                                   RegisterFailedCallback failure) {
            this->registerSuccessCallback = success;
            this->registerFailureCallback = failure;
            Node::RegisterMessage registerMessage;
            registerMessage.set_useraccount(userAccount);
            registerMessage.set_userpassword(userPassword);
            std::cout << "AuthenticationModule::" << __FUNCTION__ << "发送注册消息" << std::endl;
            this->sessionModule->sendMessage(registerMessage);
        }

        void AuthenticationModule::login(const std::string &userAccount, const std::string &userPassword,
                                         LoginSuccessCallback success,
                                         LoginFailureCallback failure) {
            this->loginSuccessCallback = success;
            this->loginFailureCallback = failure;
            Node::LoginMessage loginMessage;
            loginMessage.set_useraccount(userAccount);
            loginMessage.set_userpassword(userPassword);
            std::cout << "AuthenticationModule::" << __FUNCTION__ << "发送登陆消息" << std::endl;
            this->sessionModule->sendMessage(loginMessage);
        }


        void AuthenticationModule::setSessionModule(SessionModule &sessionModule) {
            this->sessionModule = &sessionModule;
        }

        std::list<std::string> AuthenticationModule::messageTypes() {
            std::list<std::string> messageTypeList;
            messageTypeList.push_back(Node::ResponseRegisterMessage::default_instance().GetTypeName());
            messageTypeList.push_back(Node::ResponseLoginMessage::default_instance().GetTypeName());
            return messageTypeList;
        }

        void
        AuthenticationModule::handleMessage(const ::google::protobuf::Message &message, SessionModule &sessionModule) {
            std::cout<<"AuthenticationModule::"<<__FUNCTION__<<std::endl;
            if (message.GetTypeName() == Node::ResponseLoginMessage::default_instance().GetTypeName()) {
                std::cout << "AuthenticationModule::" << __FUNCTION__ << "接收到服务器的登陆响应消息" << std::endl;
                const Node::ResponseLoginMessage &responseLoginMessage = *((const Node::ResponseLoginMessage *) &message);
                switch (responseLoginMessage.loginstate()) {
                    case Node::ResponseLoginMessage_LoginState_Success: {
                        if (this->loginSuccessCallback) {
                            this->loginSuccessCallback(*this);
                            this->loginSuccessCallback = nullptr;
                        }
                    }
                        break;
                    case Node::ResponseLoginMessage_LoginState_Failed: {
                        if (this->loginFailureCallback) {
                            switch (responseLoginMessage.failureerror()) {
                                case Node::ResponseLoginMessage_FailureError_WrongAccountOrPassword: {
                                    this->loginFailureCallback(*this, LoginFailureReasonType_WrongAccountOrPassword);
                                }
                                    break;
                                case Node::ResponseLoginMessage_FailureError_ServerInternalError: {
                                    this->loginFailureCallback(*this, LoginFailureReasonType_ServerInternalError);
                                }
                                    break;
                                default: {

                                }
                            }
                            this->loginFailureCallback = nullptr;
                        }
                    }
                        break;
                    default: {

                    }
                }
            } else if (message.GetTypeName() == Node::ResponseRegisterMessage::default_instance().GetTypeName()) {
                std::cout << "AuthenticationModule::" << __FUNCTION__ << "接收到服务器的响应注册消息" << std::endl;
                const Node::ResponseRegisterMessage &responseRegisterMessage = *((const Node::ResponseRegisterMessage *) &message);
                switch (responseRegisterMessage.registerstate()) {
                    case Node::ResponseRegisterMessage_RegisterState_Success: {
                        if (this->registerSuccessCallback) {
                            this->registerSuccessCallback(*this);
                            this->registerSuccessCallback = nullptr;
                        }
                    }
                        break;
                    case Node::ResponseRegisterMessage_RegisterState_Failed: {
                        if (this->registerFailureCallback) {
                            switch (responseRegisterMessage.failureerror()) {
                                case Node::ResponseRegisterMessage_FailureError_AccountAlreadyExists: {
                                    this->registerFailureCallback(*this,
                                                                  RegisterFailureReasonType_UserAccountDuplicate);
                                }
                                    break;
                                case Node::ResponseRegisterMessage_FailureError_ServerInternalError: {
                                    this->registerFailureCallback(*this, RegisterFailureReasonType_ServerInteralError);
                                }
                                    break;
                                default: {

                                }
                                    this->registerFailureCallback = nullptr;
                            }
                        }
                    }
                        break;
                    default: {

                    }
                }
            }
        }

    }
}
