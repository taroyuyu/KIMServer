//
// Created by Kakawater on 2018/1/29.
//
#include <stdlib.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <Client/KakaIMClient.h>
#include <Client/RosterModule/FriendRelationDelegate.h>

kakaIM::client::KakaIMClient * kakaIMClient;
class FriendRelationDelegate : public kakaIM::client::FriendRelationDelegate{
public:
    virtual void didReceiveFriendRequest(const kakaIM::client::FriendRequest & request){
        std::cout<<"FriendRelationDelegate"<<__FUNCTION__<<"收到 一条来自"<<request.getSponsorAccount()<<"的好友请求,自动接受"<<std::endl;
	kakaIMClient->andswerToFriendRequest(kakaIM::client::FriendRequestAnswer(request.getSponsorAccount(),request.getTargetAccout(),request.getApplicanId(),kakaIM::client::FriendRequestAnswerType_Accept));
    }
    virtual void didReceiveFriendRequestAnswer(const kakaIM::client::FriendRequestAnswer & answer){
        switch (answer.getAnswerType()){
            case kakaIM::client::FriendRequestAnswerType_Accept:{
                std::cout<<answer.getTargetAccout()<<"同意添加您为好友"<<std::endl;
            }
                break;
            case kakaIM::client::FriendRequesrAnswerType_Reject:{
                std::cout<<answer.getTargetAccout()<<"拒绝添加您为好友"<<std::endl;
            }
                break;
        }
    }
    virtual void didFriendShipRemoveByUser(const kakaIM::client::User & user){
        std::cout<<user.getUserAccount()<<"主动解除你们之间的好友关系"<<std::endl;
    }
};

class SingleChatMessageDelegate : public kakaIM::client::SingleChatMessageDelegate{
public:
    virtual void didReceiveSingleChatMessage(const kakaIM::client::ChatMessage & chatMessage)override {
        std::cout<<"收到来自"<<chatMessage.getSenderAccount()<<"发来的消息:"<<chatMessage.getContent()<<std::endl;
    }
};

class GroupChatMessageDelegate : public kakaIM::client::GroupChatMessageDelegate{
public:
    virtual void didReceiveGroupChatMessage(const kakaIM::client::GroupChatMessage & groupChatMessage)override {
        std::cout<<"群ID="<<groupChatMessage.getGroupId()<<" 有消息，来自"<<groupChatMessage.getSenderId()<<"发来的消息:"<<groupChatMessage.getContent()<<std::endl;
    }
};


void signIn();
void signUp();
std::shared_ptr<kakaIM::client::FriendRelationDelegate> friendRelationDelegatePtr;
std::shared_ptr<kakaIM::client::SingleChatMessageDelegate> singleChatMessageDelegatePtr;
std::shared_ptr<kakaIM::client::GroupChatMessageDelegate> groupChatMessageDelegatePtr;
int main(int argc,char * argv[])
{

    std::string serverAddr;
    uint16_t serverPort = 0;

    while (serverAddr.empty()){
        std::cout<<"请输入服务器IP地址:";
        std::cin>>serverAddr;

        if (INADDR_NONE == inet_addr(serverAddr.c_str())) {
            serverAddr = "";
        }
    }

    while (serverPort <= 0){
        std::cout<<"请输入服务器端口号:";
        std::cin>>serverPort;
    }


    kakaIM::client::KakaIMConfigs configs;
    configs.setServerAddr(serverAddr);
    configs.setServerPort(serverPort);

    kakaIMClient = kakaIM::client::KakaIMClient::create(configs);
    friendRelationDelegatePtr = std::shared_ptr<kakaIM::client::FriendRelationDelegate>(new FriendRelationDelegate());
    singleChatMessageDelegatePtr = std::shared_ptr<kakaIM::client::SingleChatMessageDelegate>(new SingleChatMessageDelegate());
    groupChatMessageDelegatePtr = std::shared_ptr<kakaIM::client::GroupChatMessageDelegate>(new GroupChatMessageDelegate());
    kakaIMClient->setFriendRelationDelegate(friendRelationDelegatePtr);
    kakaIMClient->setSingleChatMessageDelegate(singleChatMessageDelegatePtr);
    kakaIMClient->setGroupChatMessageDelegate(groupChatMessageDelegatePtr);
    while (true){
        std::cout<<"==========菜单=========="<<std::endl;
        std::cout<<"====[1]登陆"<<std::endl;
        std::cout<<"====[2]注册"<<std::endl;
        std::cout<<"====[0]退出"<<std::endl;
        std::cout<<"请选择:"<<std::endl;
        int choice = 0;
        std::cin>>choice;
        switch (choice){
            case 0:{
                return 0;
            }
            case 1:{
                signIn();
            }
                break;
            case 2:{
                signUp();
            }
        }
    }
}

void showCurrentOnlineState();
void setCurrentOnlineState();
void showCurrentUserVCard();
void setCurrentUserVCard(std::string userAccount);
void showFriendList();
void invateFriend(std::string currentUserAccout);
void deleteFriend();
void sendChatMessage();
void pullChatMessage();
void createChatGroup();
void disbandChatGroup();
void joinChatGroup();
void quitChatGroup();
void updateChatGroupInfo();
void fetchChatGroupInfo();
void fetchChatGroupMemberList();
void fetchCurrentUserChatGroupList();
void sendGroupChat(std::string userAccount);
void signIn(){
    std::string userAccount;
    std::cout<<"请输入用户名:";
    std::cin>>userAccount;
    std::string userPassword;
    std::cout<<"请输入密码:";
    std::cin>>userPassword;
    bool done = false;
    kakaIMClient->login(userAccount,userPassword,[&done](kakaIM::client::KakaIMClient & imClient){

        std::cout<<"登陆成功"<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient &imClient,kakaIM::client::KakaIMClient::LoginFailureReasonType failureReasonType){
        switch (failureReasonType){
            case kakaIM::client::KakaIMClient::LoginFailureReasonType_WrongAccountOrPassword:{
                std::cout<<"登陆失败，用户名或密码错误"<<std::endl;
            }
                break;
            case kakaIM::client::KakaIMClient::LoginFailureReasonType_NetworkError:{
                std::cout<<"登陆失败，网络错误"<<std::endl;
            }
                break;
            case kakaIM::client::KakaIMClient::LoginFailureReasonType_ServerInternalError:{
                std::cout<<"登陆失败，服务器内部错误"<<std::endl;
            }
                break;
        }
        done = true;
    });
    while (!done){
        sleep(1);
    }

    while (true){
        std::cout<<"==========菜单=========="<<std::endl;
        std::cout<<"====[1]查看当前用户的在线状态"<<std::endl;
        std::cout<<"====[2]设置当前用户的在线状态"<<std::endl;
        std::cout<<"====[3]查看当前用户的电子名片"<<std::endl;
        std::cout<<"====[4]设置当前用户的电子名片信息"<<std::endl;
	    std::cout<<"====[5]查看好友列表"<<std::endl;
	    std::cout<<"====[6]添加好友"<<std::endl;
	    std::cout<<"====[7]删除好友"<<std::endl;
	    std::cout<<"====[8]发送消息"<<std::endl;
	    std::cout<<"====[9]拉取ChatMessage"<<std::endl;
	std::cout<<"====[10]创建群"<<std::endl;
        std::cout<<"====[11]解散群"<<std::endl;
        std::cout<<"====[12]加入群"<<std::endl;
        std::cout<<"====[13]退出群"<<std::endl;
        std::cout<<"====[14]更新群资料"<<std::endl;
        std::cout<<"====[15]获取群资料"<<std::endl;
        std::cout<<"====[16]获取群成员列表"<<std::endl;
        std::cout<<"====[17]获取当前用户所加入的群"<<std::endl;
        std::cout<<"====[18]发送群消息"<<std::endl;
        std::cout<<"====[0]退出"<<std::endl;
        std::cout<<"请选择:"<<std::endl;
        int choice = 0;
        std::cin>>choice;
        switch (choice){
            case 0:{
                return;
            }
            case 1:{
                showCurrentOnlineState();
            }
                break;
            case 2:{
                setCurrentOnlineState();
            }
		   break;
            case 3:{
                showCurrentUserVCard();
            }
		   break;
            case 4:{
                setCurrentUserVCard(userAccount);
            }
		   break;
	    case 5:{
                showFriendList();
            }
                break;
	    case 6:{
                invateFriend(userAccount);
            }
		   break;
	    case 7:{
                deleteFriend();
            }
                break;
	    case 8:{
                sendChatMessage();
            }
                break;
	    case 9:{
                pullChatMessage();
            }
                break;
	    case 10:{
                createChatGroup();
            }
                break;
            case 11:{
                disbandChatGroup();
            }
                break;
            case 12:{
                joinChatGroup();
            }
                break;
            case 13:{
                quitChatGroup();
            }
                break;
            case 14:{
                updateChatGroupInfo();
            }
                break;
            case 15:{
                fetchChatGroupInfo();
            }
                break;
            case 16:{
                fetchChatGroupMemberList();
            }
                break;
            case 17:{
                fetchCurrentUserChatGroupList();
            }
                break;
            case 18:{
                sendGroupChat(userAccount);
            }
                break;
        }
    }
    
}

void showCurrentOnlineState(){
    switch(kakaIMClient->getUserOnlineState()){
        case kakaIM::client::User::UserOnlineState_Online:{
            std::cout<<"当前状态：在线"<<std::endl;
        }
            break;
        case kakaIM::client::User::UserOnlineState_Invisible:{
            std::cout<<"当前状态：隐身"<<std::endl;
        }
            break;
        case kakaIM::client::User::UserOnlineState_Offline:{
            std::cout<<"当前状态：离线"<<std::endl;
        }
            break;
    }
}
void setCurrentOnlineState(){
    std::cout<<"==========状态=========="<<std::endl;
    std::cout<<"======[1]在线"<<std::endl;
    std::cout<<"======[2]隐身"<<std::endl;
    std::cout<<"请选择:"<<std::endl;
    int choice = 0;
    std::cin>>choice;
    if(1 == choice){
        kakaIMClient->setUserOnlineState(kakaIM::client::User::UserOnlineState_Online);
    } else if (2 == choice){
        kakaIMClient->setUserOnlineState(kakaIM::client::User::UserOnlineState_Invisible);        
    }
    sleep(2);
}
void showCurrentUserVCard(){
    bool done = false;
    kakaIMClient->fetchCurrentUserVCard([&done](kakaIM::client::KakaIMClient & imClient,const kakaIM::client::UserVCard & userVcard){
        std::cout<<"获取当前用户电子名片成功: 昵称:"<<userVcard.getUserNickName()<<" 心情:"<<userVcard.getUserMood()<<std::endl;
	done = true;
    },[&done](kakaIM::client::KakaIMClient & imClient,kakaIM::client::KakaIMClient::FetchCurrentUserVCardFailureType failureTyoe){
        std::cout<<"获取当前用户电子名片失败"<<std::endl;
	done = true;
    });
    while (!done){
        sleep(1);
    }
}
void setCurrentUserVCard(std::string userAccount){
    bool done = false;
    std::string userNickName;
    std::cout<<"请输入昵称:";
    std::cin>>userNickName;
    std::string userMood;
    std::cout<<"请输入心情:";
    std::cin>>userMood;
    kakaIM::client::UserVCard userVCard(userAccount);
    userVCard.setUserNickName(userNickName);
    userVCard.setUserGender(kakaIM::client::UserVCard::UserGenderType_Male);
    userVCard.setUserMood(userMood);
    kakaIMClient->updateCurrentUserVCard(userVCard,[&done](kakaIM::client::KakaIMClient & imClient){
        std::cout<<"更新当前用户的电子名片成功"<<std::endl;
	done = true;
    },[&done](kakaIM::client::KakaIMClient & imClient,kakaIM::client::KakaIMClient::UpdateCurrentUserVCardFailureType failureType){
        std::cout<<"更新当前用户的电子名片失败"<<std::endl;
	done = true;
    });

    while (!done){
        sleep(1);
    }
}

void showFriendList(){
    bool done = false;
    kakaIMClient->fetchFriendList([&done](kakaIM::client::KakaIMClient & rosterModule,std::list<kakaIM::client::User> friendList){
        std::cout<<"============好友列表============"<<std::endl;
        for(auto user : friendList) {
            std::cout<<"===="<<user.getUserAccount()<<std::endl;
        }
	std::cout<<"好友个数:"<<friendList.size()<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient & rosterModule,kakaIM::client::KakaIMClient::FetchFriendListFailureType failureType){
        std::cout<<"获取好友列表失败";
        if(kakaIM::client::KakaIMClient::FetchFriendListFailureType_NetworkError == failureType){
            std::cout<<":网络异常";
        }else if(kakaIM::client::KakaIMClient::FetchFriendListFailureType_ServerInteralError == failureType){
            std::cout<<":服务器内部错误";
        }
        std::cout<<std::endl;
        done = true;
    });
    while (!done){
        sleep(1);
    }
}

void invateFriend(std::string currentUserAccout){
    std::string friendAccount;
    std::cout<<"请输入对方账号:";
    std::cin>>friendAccount;
    bool done = false;
    kakaIMClient->sendFriendRequest(kakaIM::client::FriendRequest(currentUserAccout,friendAccount));
    sleep(2);
}

void deleteFriend(){
    std::string friendAccount;
    std::cout<<"请输入对方账号:";
    std::cin>>friendAccount;
    bool done = false;
    kakaIMClient->deleteFriend(kakaIM::client::User(friendAccount),[&done,friendAccount](kakaIM::client::KakaIMClient & imClient){
        std::cout<<"删除好友"<<friendAccount<<"成功"<<std::endl;
        done = true;
    },[&done,friendAccount](kakaIM::client::KakaIMClient & imClient,kakaIM::client::KakaIMClient::DeleteFriendFailureType failureType){
        std::cout<<"删除好友"<<friendAccount<<"失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::DeleteFriendFailureType_ServerInteralError:{
                std::cout<<" 由于服务器内部错误"<<std::endl;
            }
                break;
            case kakaIM::client::KakaIMClient::DeleteFriendFailureType_NetworkError:{
                std::cout<<" 由于网络错误"<<std::endl;
            }
                break;
            case kakaIM::client::KakaIMClient::DeleteFriendFailureType_FriendRelationNotExitBefore:{
                std::cout<<" 因为对方本来就不是你的好友啊"<<std::endl;
            }
                break;
        }
        done = true;
    });
    while (!done){
        sleep(1);
    }
}

void sendChatMessage(){
    std::string userAccount;
    std::cout<<"请输入对方账号:";
    std::cin>>userAccount;
    std::string content;
    std::cout<<"请输入待发送的消息:";
    std::cin.get();
    std::getline(std::cin,content);
    kakaIMClient->sendChatMessage(content,userAccount);
    sleep(2);
}

void pullChatMessage(){
    kakaIMClient->pullChatMessage();
}

void createChatGroup(){
    std::string groupName;
    std::string groupDescription;
    std::cout<<"请输入群名称:";
    std::cin >> groupName;
    std::cout<<"请输入群简介:";
    std::cin >> groupDescription;
    bool done = false;
    kakaIMClient->createChatGroup(groupName,groupDescription,[&done](kakaIM::client::KakaIMClient &,std::string groupId){
        std::cout<<"========创建成功，群ID为"<<groupId<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient &,enum kakaIM::client::KakaIMClient::CreateChatGroupFailureType failureType){
        std::cout<<"========创建失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::CreateChatGroupFailureType_NetworkError:{
                std::cout<<" 由于网络异常";
            }
                break;
            case kakaIM::client::KakaIMClient::CreateChatGroupFailureType_ServerInteralError:{
                std::cout<<" 由于服务器异常";
            }
                break;
        }
        std::cout<<std::endl;
        done = true;
    });

    while (!done){
        sleep(1);
    }
}
void disbandChatGroup(){
    std::string groupID;
    std::cout<<"请输入群ID:";
    std::cin>>groupID;
    bool done = false;
    kakaIMClient->disbandChatGroup(groupID,[&done](kakaIM::client::KakaIMClient &){
        std::cout<<"========解散成功"<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient &,enum kakaIM::client::KakaIMClient::DisbandChatGroupFailureType failureType){
        std::cout<<"========解散失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::DisbandChatGroupFailureType_IllegalOperate:{
                std::cout<<" 当前用户不具备解散此群的资格";
            }
                break;
        }
        std::cout<<std::endl;
        done = true;
    });

    while (!done){
        sleep(1);
    }
}
void joinChatGroup(){
    std::string groupID;
    std::cout<<"请输入群ID:";
    std::cin>>groupID;
    bool done = false;
    kakaIMClient->joinChatGroup(groupID,[&done](kakaIM::client::KakaIMClient &){
        std::cout<<"========加入成功"<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient &,enum kakaIM::client::KakaIMClient::JoinChatGroupFailureType failureType){
        std::cout<<"========加入失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::JoinChatGroupFailureType_NotAllow:{
                std::cout<<" 不被允许";
            }
                break;
        }
        std::cout<<std::endl;
        done = true;
    });

    while (!done){
        sleep(1);
    }
}
void quitChatGroup(){
    std::string groupID;
    std::cout<<"请输入群ID:";
    std::cin>>groupID;
    bool done = false;
    kakaIMClient->quitChatGroup(groupID,[&done](kakaIM::client::KakaIMClient &){
        std::cout<<"========退出成功"<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient & ,kakaIM::client::KakaIMClient::QuitChatGroupFailureType failureType){
        std::cout<<"========退出失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::QuitChatGroupFailureType_IllegalOperate:{
                std::cout<<" 当前用户不具备退出此群的资格";
            }
                break;
        }
        std::cout<<std::endl;
        done = true;
    });

    while (!done){
        sleep(1);
    }
}
void updateChatGroupInfo(){
    std::string groupID;
    std::string groupName;
    std::string groupDescription;

    std::cout<<"请输入群ID:";
    std::cin>>groupID;
    std::cout<<"请输入群名称:";
    std::cin >> groupName;
    std::cout<<"请输入群简介:";
    std::cin >> groupDescription;
    kakaIM::client::ChatGroup group(groupID,groupName);
    group.setDescription(groupDescription);
    bool done = false;
    kakaIMClient->updateChatGroupInfo(group,[&done](kakaIM::client::KakaIMClient &){
        std::cout<<"========更新群资料成功"<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient &,enum kakaIM::client::KakaIMClient::UpdateChatGroupInfoFailureType failureType){
        std::cout<<"========更新群资料失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::UpdateChatGroupInfoFailureType_IllegalOperate:{
                std::cout<<" 当前用户不具备更新此群的资料的权限";
            }
                break;
        }
        std::cout<<std::endl;
        done = true;
    });
    while (!done){
        sleep(1);
    }
}
void fetchChatGroupInfo(){
    std::string groupID;
    std::cout<<"请输入群ID:";
    std::cin>>groupID;
    bool done = false;
    kakaIMClient->fetchChatGroupInfo(groupID,[&done](kakaIM::client::KakaIMClient &,kakaIM::client::ChatGroup group){
        std::cout<<"获取成功群资料成功"<<std::endl;
        std::cout<<"\t\t群ID="<<group.getID()<<std::endl;
        std::cout<<"\t\t群名称为:"<<group.getName()<<std::endl;
        std::cout<<"\t\t群简介为:"<<group.getDescription()<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient &,enum kakaIM::client::KakaIMClient::FetchChatGroupInfoFailureType failureType){
        std::cout<<"获取群资料失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::FetchChatGroupInfoFailureType_NetworkError:{
                std::cout<<" 由于网络异常";
            }
                break;
            case kakaIM::client::KakaIMClient::FetchChatGroupInfoFailureType_ServerInteralError:{
                std::cout<<" 由于服务器内部错误";
            }
                break;
            default:{

            }
        }
        std::cout<<std::endl;
        done = true;
    });
    while (!done){
        sleep(1);
    }
}
void fetchChatGroupMemberList(){
    std::string groupID;
    std::cout<<"请输入群ID:";
    std::cin>>groupID;
    bool done = false;
    kakaIMClient->fetchChatGroupMemberList(groupID,[&done](kakaIM::client::KakaIMClient &,const std::list<std::pair<std::string,std::string>> &memberList){
        std::cout<<"获取群成功列表成功"<<std::endl;
        for(auto item : memberList){
            std::cout<<"\t\t "<<item.first<<"\t"<<item.second<<std::endl;
        }
        std::cout<<std::endl;
        done = true;

    },[&done](kakaIM::client::KakaIMClient &,enum kakaIM::client::KakaIMClient::FetchChatGroupMemberListFailureType failureType){
        std::cout<<"获取群成员列表失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::FetchChatGroupMemberListFailureType_NetworkError:{
                std::cout<<" 由于网络异常";
            }
                break;
            case kakaIM::client::KakaIMClient::FetchChatGroupMemberListFailureType_ServerInteralError:{
                std::cout<<" 由于服务器内部错误";
            }
                break;
            case kakaIM::client::KakaIMClient::FetchChatGroupMemberListFailureType_IllegalOperate:{
                std::cout<<" 当前用户不具备此权限";
            }
                break;
            default:{

            }
        }
        std::cout<<std::endl;
        done = true;
    });

    while (!done){
        sleep(1);
    }
}
void fetchCurrentUserChatGroupList(){
    bool done = false;
    kakaIMClient->fetchCurrentUserChatGroupList([&done](kakaIM::client::KakaIMClient &,const std::list<kakaIM::client::ChatGroup> & groupList){
        std::cout<<"获取此用户所加入的群列表成功"<<std::endl;
        if(groupList.size() > 0){
            std::cout<<"\t群ID\t\t群名称"<<std::endl;
        }else{
            std::cout<<"此用户当前没有加入任何群"<<std::endl;
        }
        for(auto item : groupList){
            std::cout<<"\t"<<item.getID()<<"\t\t"<<item.getName()<<std::endl;
        }
        std::cout<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient &,enum kakaIM::client::KakaIMClient::FetchChatGroupListFailureType failureType){
        std::cout<<"获取此用户所加入的群列表失败";
        switch (failureType){
            case kakaIM::client::KakaIMClient::FetchChatGroupListFailureType_NetworkError:{
                std::cout<<" 由于网络异常";
            }
                break;
            case kakaIM::client::KakaIMClient::FetchChatGroupListFailureType_ServerInteralError:{
                std::cout<<" 由于服务器内部错误";
            };
                break;
        }
        std::cout<<std::endl;
        done = true;
    });
    while (!done){
        sleep(1);
    }
}
void sendGroupChat(std::string userAccount){
    std::string groupID;
    std::cout<<"请输入群ID:";
    std::cin>>groupID;
    std::string content;
    std::cout<<"请输入待发送的消息:";
    std::cin.get();
    std::getline(std::cin,content);
    kakaIMClient->sendGroupChatMessage(kakaIM::client::GroupChatMessage(groupID,userAccount,content));
    sleep(2);
}

void signUp(){
    std::string userAccount;
    std::cout<<"请输入用户名:";
    std::cin>>userAccount;
    std::string userPassword;
    std::cout<<"请输入密码:";
    std::cin>>userPassword;
    bool done = false;
    kakaIMClient->registerAccount(userAccount,userPassword,[&done](kakaIM::client::KakaIMClient & imClient){
        std::cout<<"===========注册成功"<<std::endl;
        done = true;
    },[&done](kakaIM::client::KakaIMClient & imClient,kakaIM::client::KakaIMClient::RegisterAccountFailureReasonType failureReasonType){
        switch (failureReasonType){
            case kakaIM::client::KakaIMClient::RegisterAccountFailureReasonType_NetworkError:{
                std::cout<<"=========注册失败"<<"网络出错"<<std::endl;
            }
                break;
            case kakaIM::client::KakaIMClient::RegisterAccountFailureReasonType_ServerInteralError:{
                std::cout<<"=========注册失败，服务器内部错误"<<std::endl;
            }
                break;
            case kakaIM::client::KakaIMClient::RegisterAccountFailureReasonType_UserAccountDuplicate:{
                std::cout<<"=========注册失败，账号已存在"<<std::endl;
            }
                break;
        }
        done = true;
    });
    while (!done){
        sleep(1);
    }
}
