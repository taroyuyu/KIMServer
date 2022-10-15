//
// Created by Kakawater on 2018/2/1.
//

#ifndef KAKAIMCLUSTER_AUTHENTICATIONMODULE_H
#define KAKAIMCLUSTER_AUTHENTICATIONMODULE_H

#include <Client/ServiceModule/ServiceModule.h>
#include <functional>

namespace kakaIM {
    namespace client {
        class AuthenticationModule : public ServiceModule {
        public:
            typedef std::function<void(AuthenticationModule & module)> RegisterSuccessCallback;
            enum RegisterFailureReasonType {
                RegisterFailureReasonType_UserAccountDuplicate,//用户名已存在
                RegisterFailureReasonType_ServerInteralError,//服务器内部错误
                RegisterFailureReasonType_NetworkError//网络连接错误
            };
            typedef std::function<void(AuthenticationModule & module,
                                       RegisterFailureReasonType failureReasonType)> RegisterFailedCallback;

            typedef std::function<void(AuthenticationModule & module)> LoginSuccessCallback;
            enum LoginFailureReasonType {
                LoginFailureReasonType_WrongAccountOrPassword,//账号或者密码错误
                LoginFailureReasonType_ServerInternalError//服务器内部错误
            };

            typedef std::function<void(AuthenticationModule & module,
                                       LoginFailureReasonType failureReasonType)> LoginFailureCallback;

            AuthenticationModule();

            ~AuthenticationModule();

            void login(const std::string &userAccount, const std::string &userPassword,
                       LoginSuccessCallback success, LoginFailureCallback failure);

            void registerAccount(std::string userAccount, std::string userPassword,
                                 RegisterSuccessCallback success,
                                 RegisterFailedCallback failure);

        protected:
            virtual void setSessionModule(SessionModule &sessionModule) override;

            virtual std::list<std::string> messageTypes() override;

            virtual void
            handleMessage(const ::google::protobuf::Message &message, SessionModule &sessionModule) override;

            RegisterSuccessCallback registerSuccessCallback;
            RegisterFailedCallback registerFailureCallback;
            LoginSuccessCallback loginSuccessCallback;
            LoginFailureCallback loginFailureCallback;
        private:
            SessionModule *sessionModule;
        };
    }
}


#endif //KAKAIMCLUSTER_AUTHENTICATIONMODULE_H
