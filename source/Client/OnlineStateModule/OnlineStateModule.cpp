//
// Created by Kakawater on 2018/2/16.
//

#include "OnlineStateModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"

namespace kakaIM {
    namespace client {
        OnlineStateModule::OnlineStateModule():sessionModule(nullptr){

        }
        OnlineStateModule::~OnlineStateModule() {

        }
        void OnlineStateModule::setSessionModule(SessionModule &sessionModule){
            this->sessionModule = &sessionModule;
        }

        std::list<std::string> OnlineStateModule::messageTypes() {
            std::list<std::string> messageTypeList;
            messageTypeList.push_back(Node::OnlineStateMessage::default_instance().GetTypeName());
            return messageTypeList;
        }

	void OnlineStateModule::handleMessage(const ::google::protobuf::Message & message,SessionModule & sessionModule){
            if(message.GetTypeName() == Node::OnlineStateMessage::default_instance().GetTypeName()) {
                const Node::OnlineStateMessage & onlineStateMessage = *((const Node::OnlineStateMessage*)&message);
                switch (onlineStateMessage.userstate()){
                    case Node::OnlineStateMessage_OnlineState_Online:{
                        this->userOnlineDB[onlineStateMessage.useraccount()] = User::UserOnlineState_Online;
                    }
                        break;
                    case Node::OnlineStateMessage_OnlineState_Invisible:{
                        this->userOnlineDB[onlineStateMessage.useraccount()] = User::UserOnlineState_Invisible;
                    }
                        break;
                    case Node::OnlineStateMessage_OnlineState_Offline:{
                        this->userOnlineDB[onlineStateMessage.useraccount()] = User::UserOnlineState_Offline;
                    }
                        break;
                    default:{

                    }
                }
            }
        }

        User::UserOnlineState OnlineStateModule::getUserOnlineState(std::string userAccount){
            auto userOnlineStateIt = this->userOnlineDB.find(userAccount);
            if(userOnlineStateIt != this->userOnlineDB.end()){
                return userOnlineStateIt->second;
            }else{
                return User::UserOnlineState_Offline;
            }
        }
    }
}
