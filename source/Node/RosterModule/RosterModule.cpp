//
// Created by Kakawater on 2018/1/7.
//

#include <zconf.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "RosterModule.h"
#include "../Log/log.h"
#include "../../Common/util/Date.h"
#include "../../Common/proto/KakaIMRPC.pb.h"
#include "../../Common/proto/KakaIMMessage.pb.h"

namespace kakaIM {
    namespace node {
        RosterModule::RosterModule() : mEpollInstance(-1), messageEventfd(-1), m_dbConnection(nullptr) {
            this->logger = log4cxx::Logger::getLogger(RosterModuleLogger);
        }

        RosterModule::~RosterModule() {
            if (this->m_dbConnection && this->m_dbConnection->is_open()) {
                this->m_dbConnection->disconnect();
            }
        }

        bool RosterModule::init() {
            //创建eventfd,并提供信号量语义
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }

            //创建Epoll实例
            if (-1 == (this->mEpollInstance = epoll_create1(0))) {
                return false;
            }

            //向Epill实例注册messageEventfd,clusterMessageEventfd
            struct epoll_event messageEventfdEvent;
            messageEventfdEvent.events = EPOLLIN;
            messageEventfdEvent.data.fd = this->messageEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->messageEventfd, &messageEventfdEvent)) {
                return false;
            }

            //创建AMQP信道
            this->mAmqpChannel = AmqpClient::Channel::Create("111.230.5.199", 5672, "kakaIM-node", "kakaIM-node_aixocm",
                                                             "KakaIM");

            return true;
        }

        void RosterModule::start() {
            if (false == this->m_isStarted) {
                this->m_isStarted = true;
                this->m_workThread = std::move(std::thread([this]() {
                    this->execute();
                    this->m_isStarted = false;
                }));
                this->mRosterRPCWorkThread = std::move(std::thread([this]() {
                    this->rosterRPCListenerWork();
                    this->m_isStarted = false;
                }));
            }
        }

        void RosterModule::execute() {
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }

            while (not this->m_needStop) {
                if (auto task = this->mTaskQueue.try_pop()) {
                    this->dispatchMessage(*task);
                } else {
                    std::this_thread::yield();
                }
            }

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }

        void RosterModule::shouldStop() {
            this->m_needStop = true;
        }

        void RosterModule::dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, std::string> &task) {
            auto messageType = task.first->GetTypeName();
            if (messageType ==
                kakaIM::Node::BuildingRelationshipRequestMessage::default_instance().GetTypeName()) {
                handleBuildingRelationshipRequestMessage(
                        *(kakaIM::Node::BuildingRelationshipRequestMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::BuildingRelationshipAnswerMessage::default_instance().GetTypeName()) {
                handleBuildingRelationshipAnswerMessage(
                        *(kakaIM::Node::BuildingRelationshipAnswerMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::DestroyingRelationshipRequestMessage::default_instance().GetTypeName()) {
                handleDestroyingRelationshipRequestMessage(
                        *(kakaIM::Node::DestroyingRelationshipRequestMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::FriendListRequestMessage::default_instance().GetTypeName()) {
                handleFriendListRequestMessage(
                        *(kakaIM::Node::FriendListRequestMessage *) task.first.get(),
                        task.second);
            } else if (messageType ==
                       kakaIM::Node::FetchUserVCardMessage::default_instance().GetTypeName()) {
                handleFetchUserVCardMessage(*(kakaIM::Node::FetchUserVCardMessage *) task.first.get(),
                                            task.second);
            } else if (messageType ==
                       kakaIM::Node::UpdateUserVCardMessage::default_instance().GetTypeName()) {
                handleUpdateUserVCardMessage(*(kakaIM::Node::UpdateUserVCardMessage *) task.first.get(),
                                             task.second);
            }
        }

        void RosterModule::rosterRPCListenerWork() {
            //配置AMQP信道
            this->mAmqpChannel->DeclareExchange("KakaIMRPC");
            this->mAmqpChannel->DeclareQueue("KakaIMRosterService", false, false, false, true);
            this->mAmqpChannel->BindQueue("KakaIMRosterService", "KakaIMRPC", "KakaIMRPC_FriendList");
            std::string consumer_tag = this->mAmqpChannel->BasicConsume("KakaIMRosterService", "", true, false, false,
                                                                        1);
            while (this->m_isStarted) {
                AmqpClient::Envelope::ptr_t envelope = this->mAmqpChannel->BasicConsumeMessage(consumer_tag);
                LOG4CXX_DEBUG(this->logger, typeid(this).name() << "" << __FUNCTION__);
                rpc::FriendListRequestMessage friendListRequestMessage;
                friendListRequestMessage.ParseFromString(envelope->Message()->Body());
                rpc::FriendListResponseMessage friendListResponseMessage;

                friendListResponseMessage.set_useraccount(friendListRequestMessage.useraccount());
                if (friendListRequestMessage.IsInitialized()) {
                    this->mAmqpChannel->BasicAck(envelope);
                    const std::string userAccount = friendListRequestMessage.useraccount();
                    const uint64_t peerCurrentVersion = friendListRequestMessage.currentversion();
                    uint64_t currentVersion = 0;
                    switch (this->fetchFriendListVersion(userAccount, currentVersion)) {
                        case FetchFriendListVersionResult_Success: {
                            if (currentVersion > peerCurrentVersion) {
                                friendListResponseMessage.set_currentversion(currentVersion);
                                std::set<std::string> friendList;
                                if (FetchFriendListResult_Success == this->fetchFriendList(userAccount, friendList)) {
                                    friendListResponseMessage.set_status(rpc::FriendListResponseMessage_Status_Success);
                                    friendListResponseMessage.set_currentversion(currentVersion);
                                    for (std::string friendAccount: friendList) {
                                        friendListResponseMessage.add_friend_()->set_friendaccount(friendAccount);
                                    }
                                } else {
                                    friendListResponseMessage.set_status(rpc::FriendListResponseMessage_Status_Failed);
                                    friendListResponseMessage.set_failureerror(
                                            rpc::FriendListResponseMessage_FailureType_ServerInternalError);
                                }

                            } else {
                                friendListResponseMessage.set_currentversion(currentVersion);
                                friendListResponseMessage.set_status(
                                        rpc::FriendListResponseMessage_Status_SuccessButNoNewChange);
                            }
                        }
                            break;
                        case FetchFriendListVersionResult_RecordNotExist: {
                            friendListResponseMessage.set_status(rpc::FriendListResponseMessage_Status_Failed);
                            friendListResponseMessage.set_failureerror(
                                    rpc::FriendListResponseMessage_FailureType_RecordNotExist);
                        }
                            break;
                        case FetchFriendListVersionResult_DBConnectionNotExit:
                        case FetchFriendListVersionResult_InteralError:
                        default: {
                            friendListResponseMessage.set_status(rpc::FriendListResponseMessage_Status_Failed);
                            friendListResponseMessage.set_failureerror(
                                    rpc::FriendListResponseMessage_FailureType_ServerInternalError);
                        }
                            break;
                    }
                    AmqpClient::BasicMessage::ptr_t answerMessage = AmqpClient::BasicMessage::Create(
                            friendListResponseMessage.SerializeAsString());
                    answerMessage->ReplyTo(envelope->Message()->ReplyTo());
                    answerMessage->CorrelationId(envelope->Message()->CorrelationId());
                    this->mAmqpChannel->BasicPublish("KakaIMRPC", envelope->Message()->ReplyTo(), answerMessage);

                } else {
                    LOG4CXX_DEBUG(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 消息格式不正确");
                    this->mAmqpChannel->BasicReject(envelope, false);
                }
            }
        }

        void RosterModule::handleBuildingRelationshipRequestMessage(
                const kakaIM::Node::BuildingRelationshipRequestMessage &message,
                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            //1.查询SessionID对应的userAccount
            std::string userAccount = this->mQueryUserAccountWithSessionServicePtr.lock()->queryUserAccountWithSession(
                    message.sessionid());
            //2.比较发起者与userAccount是否相等
            if (userAccount != message.sponsoraccount()) {
                return;
            }

            kakaIM::Node::BuildingRelationshipRequestMessage requestMessage(message);
            uint64_t applicant_id;
            std::string submissionTime;
            //3.将此申请保存到数据库
            const std::string AddUserRelationApplicationStatement = "AddUserRelationApplication";
            const std::string addUserRelationApplicationSQL = "INSERT INTO user_relation_applications (sponsor, target, state, submission_time, introduction, applicant_id)"
                                                              "VALUES ($1, $2, 'Pending', now(), $3, DEFAULT)RETURNING applicant_id,submission_time;";
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                //异常处理
                return;;
            }

            try {

                pqxx::work dbWork(*dbConnection);

                auto addUserRelationApplicationStatementInvocation = dbWork.prepared(
                        AddUserRelationApplicationStatement);

                if (!addUserRelationApplicationStatementInvocation.exists()) {
                    dbConnection->prepare(AddUserRelationApplicationStatement, addUserRelationApplicationSQL);
                }

                pqxx::result result = addUserRelationApplicationStatementInvocation(message.sponsoraccount())(
                        message.targetaccount())(message.introduction()).exec();

                applicant_id = result[0][0].as<uint64_t>();
                submissionTime = result[0][1].as<std::string>();
                submissionTime = kaka::Date(submissionTime).toString();

                requestMessage.set_applicant_id(applicant_id);
                requestMessage.set_submissiontime(submissionTime);

                //提交事务
                dbWork.commit();

            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "将好友申请插入数据库失败,"
                                                  << exception.what());
            }

            //4.发送此消息
            auto messageSendServicePtr = this->mMessageSendServicePtr.lock();
            if (messageSendServicePtr) {
                //4.1发送给接收方
                messageSendServicePtr->sendMessageToUser(requestMessage.targetaccount(), requestMessage);
                //4.2发送给发送方
                messageSendServicePtr->sendMessageToUser(requestMessage.sponsoraccount(), requestMessage);
            } else {
                LOG4CXX_WARN(this->logger,
                             typeid(this).name() << "" << __FUNCTION__ << "处理失败，由于不存在messageSendService");
            }
        }

        bool
        RosterModule::checkUserRelationApplication(const std::string sponsorAccount, const std::string targetAccount,
                                                   const uint64_t applicantId) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            static const std::string CheckApplicantIdStatement = "CheckApplicantId";
            static const std::string checkApplicantIdSQL = "SELECT sponsor,target,state FROM user_relation_applications WHERE applicant_id = $1;";

            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 获取数据库连接失败" << errno);
                //异常处理
                return false;;
            }
            std::string sponsor;
            std::string target;
            bool applicantion_is_Pending = false;
            try {

                pqxx::work dbWork(*dbConnection);

                auto checkApplicantIdStatementInvocation = dbWork.prepared(CheckApplicantIdStatement);

                if (!checkApplicantIdStatementInvocation.exists()) {
                    dbConnection->prepare(CheckApplicantIdStatement, checkApplicantIdSQL);
                }

                pqxx::result result = checkApplicantIdStatementInvocation(applicantId).exec();

                //提交事务
                dbWork.commit();

                if (1 != result.size()) {
                    return false;
                }

                auto record = result[0];

                sponsor = record[record.column_number("sponsor")].as<std::string>();
                target = record[record.column_number("target")].as<std::string>();

                applicantion_is_Pending = record[record.column_number("state")].as<std::string>() == "Pending";

            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "验证好友申请失败," << exception.what());
                return false;
            }

            if (applicantion_is_Pending && sponsor == sponsorAccount && target == targetAccount) {
                return true;
            } else {
                return false;
            }

        }

        std::list<std::string> RosterModule::retriveUserFriendList(const std::string userAccount) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);

            static const std::string FetchUserFriendListStatement = "FetchUserFriendList";
            static const std::string fetchUserFriendListSQL = "SELECT sponsor,target FROM user_relationship WHERE sponsor = $1 OR target = $1;";
            auto dbConnection = this->getDBConnection();
            std::list<std::string> friendList;
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                //异常处理
                return friendList;
            }

            try {
                pqxx::work dbWork(*dbConnection);

                auto fetchUserFriendListStatementInvocation = dbWork.prepared(FetchUserFriendListStatement);
                if (!fetchUserFriendListStatementInvocation.exists()) {
                    dbConnection->prepare(FetchUserFriendListStatement, fetchUserFriendListSQL);
                }

                pqxx::result result = fetchUserFriendListStatementInvocation(userAccount).exec();
                dbWork.commit();

                for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                    for (auto field = row->begin(); field != row->end(); ++field) {

                        if (userAccount != field->c_str()) {
                            friendList.push_back(field->c_str());
                        }
                    }
                }

            } catch (std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 查询用户" << userAccount << "的好友列表失败,"
                                                  << exception.what());
            }

            return friendList;
        }

        bool RosterModule::updateUserRelationApplicationState(const uint64_t applicantId,
                                                              kakaIM::Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer answer,
                                                              std::string &handleTime) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            std::string state = "Pending";
            switch (answer) {
                case Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Accept: {
                    state = "Allow";
                }
                    break;
                case Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Reject: {
                    state = "Reject";
                }
                    break;
                default: {
                    return false;
                }
            }

            static const std::string UpdateUserRelationApplicationStateStatement = "UpdateUserRelationApplicationState";
            static const std::string updateUserRelationApplicationStateSQL = "UPDATE user_relation_applications SET state = $1 , handle_time = now() WHERE applicant_id = $2 RETURNING handle_time;";


            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接失败");
                //异常处理
                return false;;
            }
            try {

                pqxx::work dbWork(*dbConnection);

                auto updateUserRelationApplicationStateStatementInvocation = dbWork.prepared(
                        UpdateUserRelationApplicationStateStatement);

                if (!updateUserRelationApplicationStateStatementInvocation.exists()) {
                    dbConnection->prepare(UpdateUserRelationApplicationStateStatement,
                                          updateUserRelationApplicationStateSQL);
                }

                pqxx::result result = updateUserRelationApplicationStateStatementInvocation(state)(applicantId).exec();


                if (1 != result.affected_rows()) {
                    return false;
                }

                handleTime = result[0][0].as<std::string>();

                //提交事务
                dbWork.commit();

            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "更新好友申请状态失败," << exception.what());
                return false;
            }

            return true;
        }

        void RosterModule::handleBuildingRelationshipAnswerMessage(
                const kakaIM::Node::BuildingRelationshipAnswerMessage &message,
                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            kakaIM::Node::BuildingRelationshipAnswerMessage answerMessage(message);
            //1.查询SessionID对应的userAccount
            std::string userAccount = this->mQueryUserAccountWithSessionServicePtr.lock()->queryUserAccountWithSession(
                    answerMessage.sessionid());
            //2.比较发起者与userAccount是否相等
            if (userAccount != answerMessage.targetaccount()) {
                return;
            }//3.检查数据库中是否存在此applicationId
            if (false ==
                this->checkUserRelationApplication(answerMessage.sponsoraccount(), answerMessage.targetaccount(),
                                                   answerMessage.applicant_id())) {
                return;;
            }
            //4.设置好友申请状态
            std::string handleTime;
            this->updateUserRelationApplicationState(answerMessage.applicant_id(), answerMessage.answer(), handleTime);
            handleTime = kaka::Date(handleTime).toString();

            answerMessage.set_handletime(handleTime);
            switch (message.answer()) {
                case kakaIM::Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Accept: {


                    auto dbConnection = this->getDBConnection();
                    if (!dbConnection) {
                        LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接失败");
                        //异常处理
                        return;;
                    }
                    pqxx::work dbWork(*dbConnection);

                    const std::string BuildRelationShipSQLStatement = "BuildRelationShipSQL";
                    const std::string buildRelationShipSQL = "INSERT INTO user_relationship "
                                                             "(sponsor, target, relation, created_at) "
                                                             "VALUES ($1,$2,'Friend',now())";

                    try {

                        auto buildRelationShipSQLStatementInvocation = dbWork.prepared(BuildRelationShipSQLStatement);

                        if (!buildRelationShipSQLStatementInvocation.exists()) {
                            dbConnection->prepare(BuildRelationShipSQLStatement, buildRelationShipSQL);
                        }

                        pqxx::result result = buildRelationShipSQLStatementInvocation(answerMessage.sponsoraccount())(
                                answerMessage.targetaccount()).exec();

                        //提交事务
                        dbWork.commit();

                        if (1 == result.affected_rows()) {//好友关系建立成功
                            uint64_t sponsorFriendListVersion = 0;
                            uint64_t targetFriendListVersion = 0;
                            this->updateFriendListVersion(answerMessage.sponsoraccount(), sponsorFriendListVersion);
                            this->updateFriendListVersion(answerMessage.targetaccount(), targetFriendListVersion);

                            auto messageSendServicePtr = this->mMessageSendServicePtr.lock();
                            if (messageSendServicePtr) {
                                messageSendServicePtr->sendMessageToUser(answerMessage.targetaccount(), answerMessage);
                                messageSendServicePtr->sendMessageToUser(answerMessage.sponsoraccount(), answerMessage);
                            } else {

                            }
                        } else {//好友关系建立失败

                        }
                    } catch (const std::exception &exception) {
                        LOG4CXX_ERROR(this->logger,
                                      typeid(this).name() << "" << __FUNCTION__ << "好友关系建立失败,"
                                                          << exception.what());
                    }

                }
                    break;
                case kakaIM::Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Reject: {
                    auto messageSendServicePtr = this->mMessageSendServicePtr.lock();
                    if (messageSendServicePtr) {
                        messageSendServicePtr->sendMessageToUser(answerMessage.sponsoraccount(), answerMessage);
                    } else {

                    }
                }
                    break;
                default: {

                }
                    break;
            }
        }

        RosterModule::UpdateFriendListVersionResult
        RosterModule::updateFriendListVersion(const std::string userAccount, uint64_t &currentVersion) {
            //1.查询用户当前的好友列表版本
            uint64_t friendListVersion = 0;
            switch (this->fetchFriendListVersion(userAccount, friendListVersion)) {
                case FetchFriendListVersionResult_Success: {
                    static const std::string UpdateFriendListVersionStatement = "UpdateFriendListVersion";
                    static const std::string UpdateFriendListVersionSQL = "UPDATE friendlist_version SET current_version = (SELECT current_version FROM friendlist_version WHERE  user_account = $1) + 1 WHERE user_account = $1 RETURNING  current_version;";

                    auto dbConnection = this->getDBConnection();
                    if (!dbConnection) {
                        LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                        //异常处理
                        return UpdateFriendListVersionResult_DBConnectionNotExit;
                    }

                    pqxx::work dbWork(*dbConnection);

                    auto updateFriendListVersionStatementInvocation = dbWork.prepared(UpdateFriendListVersionStatement);
                    if (!updateFriendListVersionStatementInvocation.exists()) {
                        dbConnection->prepare(UpdateFriendListVersionStatement, UpdateFriendListVersionSQL);
                    }

                    pqxx::result result = updateFriendListVersionStatementInvocation(userAccount).exec();
                    dbWork.commit();

                    if (result.affected_rows()) {
                        currentVersion = result[0][0].as<uint64_t>();
                        return UpdateFriendListVersionResult_Success;
                    } else {
                        return UpdateFriendListVersionResult_InteralError;
                    }

                }
                    break;
                case FetchFriendListVersionResult_RecordNotExist: {
                    static const std::string AddFriendListVersionRecordStatement = "AddFriendListVersionRecord";
                    static const std::string AddFriendListVersionRecordSQL = "INSERT INTO friendlist_version (user_account, current_version) VALUES ($1,1)  RETURNING  current_version;";

                    auto dbConnection = this->getDBConnection();
                    if (!dbConnection) {
                        LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                        //异常处理
                        return UpdateFriendListVersionResult_DBConnectionNotExit;
                    }

                    pqxx::work dbWork(*dbConnection);
                    auto addFriendListVersionRecordStatementInvocation = dbWork.prepared(
                            AddFriendListVersionRecordStatement);
                    if (!addFriendListVersionRecordStatementInvocation.exists()) {
                        dbConnection->prepare(AddFriendListVersionRecordStatement, AddFriendListVersionRecordSQL);
                    }

                    pqxx::result result = addFriendListVersionRecordStatementInvocation(userAccount).exec();
                    dbWork.commit();

                    if (result.affected_rows()) {
                        currentVersion = result[0][0].as<uint64_t>();
                        return UpdateFriendListVersionResult_Success;
                    } else {
                        return UpdateFriendListVersionResult_InteralError;
                    }
                }
                    break;

                case FetchFriendListVersionResult_DBConnectionNotExit: {
                    return UpdateFriendListVersionResult_DBConnectionNotExit;
                }
                case FetchFriendListVersionResult_InteralError:
                default: {
                    return UpdateFriendListVersionResult_InteralError;
                }
            }
        }

        RosterModule::FetchFriendListVersionResult
        RosterModule::fetchFriendListVersion(const std::string userAccount, uint64_t &friendListVersion) {

            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                //异常处理
                return FetchFriendListVersionResult_DBConnectionNotExit;
            }

            try {
                pqxx::work dbWork(*dbConnection);

                static const std::string QueryFriendListVersionStatement = "QueryFriendListVersion";
                static const std::string QueryFriendListVersionSQL = "SELECT current_version FROM friendlist_version WHERE user_account = $1;";

                auto queryFriendListVersionStatementInvocation = dbWork.prepared(QueryFriendListVersionStatement);
                if (!queryFriendListVersionStatementInvocation.exists()) {
                    dbConnection->prepare(QueryFriendListVersionStatement, QueryFriendListVersionSQL);
                }

                pqxx::result result = queryFriendListVersionStatementInvocation(userAccount).exec();
                dbWork.commit();

                if (result.size()) {
                    friendListVersion = result[0][0].as<uint64_t>();
                    return FetchFriendListVersionResult_Success;
                } else {
                    return FetchFriendListVersionResult_RecordNotExist;
                }

            } catch (std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 查询用户" << userAccount << "的好友列表版本失败,"
                                                  << exception.what());
                return FetchFriendListVersionResult_InteralError;
            }

        }

        void RosterModule::handleDestroyingRelationshipRequestMessage(
                const kakaIM::Node::DestroyingRelationshipRequestMessage &message,
                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            kakaIM::Node::DestoryingRelationshipResponseMessage responseMessage;
            responseMessage.set_sponsoraccount(message.sponsoraccount());
            responseMessage.set_targetaccount(message.targetaccount());
            if (message.has_sign()) {
                responseMessage.set_sign(message.sign());
            }

            //1.查询SessionID对应的userAccount
            std::string userAccount = this->mQueryUserAccountWithSessionServicePtr.lock()->queryUserAccountWithSession(
                    message.sessionid());
            //2.比较发起者与userAccount是否相等
            if (userAccount != message.sponsoraccount()) {
                responseMessage.set_response(
                        kakaIM::Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_IllegalOperation);
                responseMessage.set_sessionid(message.sessionid());
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseMessage);
                }
                LOG4CXX_ERROR(this->logger, __FUNCTION__ << " connectionOperationService不存在");
                return;
            }

            const std::string DestoryRelationShipSQLStatement = "DestoryRelationShipSQL";
            const std::string destoryRelationShipSQL = "DELETE FROM user_relationship WHERE sponsor =  (SELECT sponsor FROM user_relationship WHERE (sponsor= $1 AND target= $2 ) OR (sponsor = $2  AND target= $1 ));";

            try {

                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                    responseMessage.set_response(
                            kakaIM::Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_ServerInteralError);
                    responseMessage.set_sessionid(message.sessionid());
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseMessage);
                    }
                    return;;
                }
                pqxx::work dbWork(*dbConnection);

                auto destoryRelationShipSQLStatementInvocation = dbWork.prepared(DestoryRelationShipSQLStatement);

                if (!destoryRelationShipSQLStatementInvocation.exists()) {
                    dbConnection->prepare(DestoryRelationShipSQLStatement, destoryRelationShipSQL);
                }

                pqxx::result result = destoryRelationShipSQLStatementInvocation(message.sponsoraccount())(
                        message.targetaccount()).exec();

                dbWork.commit();

                uint64_t sponsorFriendListVersion = 0;
                uint64_t targetFriendListVersion = 0;
                this->updateFriendListVersion(message.sponsoraccount(), sponsorFriendListVersion);
                this->updateFriendListVersion(message.targetaccount(), targetFriendListVersion);

                responseMessage.set_response(
                        kakaIM::Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_Success);

                auto messageSendServicePtr = this->mMessageSendServicePtr.lock();
                if (messageSendServicePtr) {
                    messageSendServicePtr->sendMessageToUser(responseMessage.sponsoraccount(), responseMessage);
                    messageSendServicePtr->sendMessageToUser(responseMessage.targetaccount(), responseMessage);
                } else {
                    LOG4CXX_ERROR(this->logger, __FUNCTION__ << " messageSendServicePtr不存在");
                }

            } catch (pqxx::sql_error exception) {//拆除失败
                responseMessage.set_response(
                        kakaIM::Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_ServerInteralError);
                responseMessage.set_sessionid(message.sessionid());
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseMessage);
                } else {
                    LOG4CXX_ERROR(this->logger, __FUNCTION__ << " connectionOperationService不存在");
                }
            }
        }

        RosterModule::FetchFriendListResult
        RosterModule::fetchFriendList(const std::string userAccount, std::set<std::string> &friendList) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);

            const std::string QueryFriendListSQLStatement = "QueryFriendListSQL";
            const std::string queryFriendListSQL = "SELECT sponsor,target FROM user_relationship WHERE sponsor = $1 OR target = $1 ;";

            try {

                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_WARN(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 获取数据库连接失败");
                    //异常处理
                    return FetchFriendListResult_DBConnectionNotExit;
                }

                pqxx::work dbWork(*dbConnection);

                //查询好友列表
                auto queryFriendListSQLStatementInvocation = dbWork.prepared(QueryFriendListSQLStatement);
                if (!queryFriendListSQLStatementInvocation.exists()) {
                    dbConnection->prepare(QueryFriendListSQLStatement, queryFriendListSQL);
                }

                pqxx::result result = queryFriendListSQLStatementInvocation(userAccount).exec();

                for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                    for (auto field = row->begin(); field != row->end(); ++field) {

                        if (userAccount != field->c_str()) {

                            friendList.insert(field->c_str());
                        }
                    }
                }
                dbWork.commit();

                return FetchFriendListResult_Success;
            } catch (std::exception &exception) {//查询失败
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 查询好友列表出错," << exception.what());
                return FetchFriendListResult_InteralError;
            }

        }

        void RosterModule::handleFriendListRequestMessage(const kakaIM::Node::FriendListRequestMessage &message,
                                                          const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            auto queryUserAccountWithSessionServicePtr = this->mQueryUserAccountWithSessionServicePtr.lock();
            std::string userAccount;
            if (queryUserAccountWithSessionServicePtr) {
                userAccount = queryUserAccountWithSessionServicePtr->queryUserAccountWithSession(message.sessionid());
            } else {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                << "因queryUserAccountWithSessionService不存在,无法处理");
                return;
            }
            uint64_t clientFriendListVersion = 0;
            if (message.has_currentversion()) {
                clientFriendListVersion = message.currentversion();
            }

            kakaIM::Node::FriendListResponseMessage responseMessage;
            responseMessage.set_sessionid(message.sessionid());

            uint64_t currentVersion = 0;
            switch (this->fetchFriendListVersion(userAccount, currentVersion)) {
                case FetchFriendListVersionResult_Success: {
                    if (currentVersion > clientFriendListVersion) {
                        std::set<std::string> friendList;
                        if (FetchFriendListResult_Success == this->fetchFriendList(userAccount, friendList)) {
                            for (std::string friendAccount: friendList) {
                                auto friendItem = responseMessage.add_friend_();
                                friendItem->set_friendaccount(friendAccount);
                            }
                        }

                    }
                }
                case FetchFriendListVersionResult_RecordNotExist: {
                    responseMessage.set_currentversion(currentVersion);
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseMessage);
                    }
                }
                    break;
                case FetchFriendListVersionResult_DBConnectionNotExit:
                case FetchFriendListVersionResult_InteralError:
                default: {
                }
                    break;
            }
        }

        void RosterModule::handleFetchUserVCardMessage(const kakaIM::Node::FetchUserVCardMessage &message,
                                                       const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            kakaIM::Node::UserVCardResponseMessage responseMessage;
            responseMessage.set_sessionid(message.sessionid());

            kaka::Date clientVersion = kaka::Date::getCurrentDate();
            if (message.has_currentversion()) {
                clientVersion = kaka::Date(message.currentversion());
            }

            const std::string QueryUserVCardSQLStatement = "QueryUserVCardSQL";
            const std::string queryUserVCardSQL = "SELECT nickname,sex,avator,mood,mtime FROM user_vcard WHERE account = $1 ;";

            try {
                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 连接数据库失败");
                    //异常处理
                    return;;
                }
                pqxx::work dbWork(*dbConnection);

                auto queryUserVCardSQLStatementInvocation = dbWork.prepared(QueryUserVCardSQLStatement);
                if (!queryUserVCardSQLStatementInvocation.exists()) {
                    dbConnection->prepare(QueryUserVCardSQLStatement, queryUserVCardSQL);
                }
                pqxx::result result = queryUserVCardSQLStatementInvocation(message.userid()).exec();

                if (1 != result.size()) {
                    throw std::exception();
                }

                pqxx::result::const_iterator userVcardRecord = result.begin();

                auto field = userVcardRecord->begin();
                std::string userNickName = field->c_str();
                ++field;
                kakaIM::Node::UserGenderType userGenderType =
                        field->c_str() == std::string("male") ? kakaIM::Node::Male : kakaIM::Node::Female;
                ++field;

                pqxx::binarystring userAvatorBlob(userVcardRecord["avator"]);
                std::string userAvator((char *) userAvatorBlob.data(), userAvatorBlob.size());

                ++field;
                std::string userMood = field->c_str();

                ++field;
                kaka::Date mtime = kaka::Date(field->c_str());

                responseMessage.set_userid(message.userid());
                responseMessage.set_currentversion(mtime.toString());

                if (clientVersion == mtime) {

                } else {
                    responseMessage.set_nickname(userNickName);
                    responseMessage.set_gender(userGenderType);
                    responseMessage.set_mood(userMood);
                    responseMessage.set_avator(userAvator);
                }

                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseMessage);
                }

            } catch (std::exception exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 查询用户:" << message.userid()
                                                  << "的电子名片信息失败,"
                                                  << exception.what());
            }
        }

        void RosterModule::handleUpdateUserVCardMessage(const kakaIM::Node::UpdateUserVCardMessage &message,
                                                        const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            auto queryUserAccountWithSessionServicePtr = this->mQueryUserAccountWithSessionServicePtr.lock();
            std::string userAccount;
            if (queryUserAccountWithSessionServicePtr) {
                userAccount = queryUserAccountWithSessionServicePtr->queryUserAccountWithSession(message.sessionid());
            } else {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                << "因queryuseraccountwithsessionservice不存在，无法处理");
                return;
            }
            std::string userNickName = message.nickname();
            kakaIM::Node::UserGenderType userGender = message.gender();
            std::string userMood = message.mood();
            std::string userAvator = message.avator();
            kakaIM::Node::UpdateUserVCardMessageResponse response;
            response.set_sessionid(message.sessionid());

            const std::string UpdateUserVCardSQLStatement = "UpdateUserVCardSQL";
            const std::string updateUserVCardSQL = "UPDATE user_vcard SET nickname = $1 , sex = $2 , avator = $3 , mood = $4,mtime = now() WHERE account = $5 ;";

            try {
                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                    //异常处理
                    return;;
                }
                pqxx::work dbWork(*dbConnection);

                auto updateUserVCardSQLStatementInvocation = dbWork.prepared(UpdateUserVCardSQLStatement);

                if (!updateUserVCardSQLStatementInvocation.exists()) {
                    dbConnection->prepare(UpdateUserVCardSQLStatement, updateUserVCardSQL);
                }

                pqxx::binarystring userAvatorBinary(userAvator.data(), userAvator.size());
                pqxx::result result = updateUserVCardSQLStatementInvocation(userNickName)(
                        std::string(userGender == kakaIM::Node::Male ? "male" : "female"))(userAvatorBinary)(userMood)(
                        userAccount).exec();

                if (1 != result.affected_rows()) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "更新用户电子名片失败");
                }

                dbWork.commit();

                response.set_state(kakaIM::Node::UpdateUserVCardMessageResponse_UpdateUserVCardStateType_Success);

            } catch (std::exception &exception) {
                LOG4CXX_WARN(this->logger,
                             typeid(this).name() << "" << __FUNCTION__ << "更新用户电子名片失败," << exception.what());
                response.set_state(kakaIM::Node::UpdateUserVCardMessageResponse_UpdateUserVCardStateType_Failure);
            }

            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, response);
            }
        }

        bool RosterModule::checkFriendRelation(const std::string userA, const std::string userB) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            static const std::string CheckFriendRelationStatement = "CheckFriendRelation";
            static const std::string checkFriendRelationSQL = "SELECT sponsor,target FROM user_relationship WHERE (sponsor = $1 AND target = $2) OR (sponsor = $2 AND target = $1);";
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "获取数据库连接出错");
                //异常处理
                return false;;
            }
            try {
                pqxx::work dbWork(*dbConnection);

                auto checkFriendRelationStatementInvocation = dbWork.prepared(CheckFriendRelationStatement);
                if (!checkFriendRelationStatementInvocation.exists()) {
                    dbConnection->prepare(CheckFriendRelationStatement, checkFriendRelationSQL);
                }

                pqxx::result result = checkFriendRelationStatementInvocation(userA)(userB).exec();
                dbWork.commit();

                if (result.size() >= 1) {
                    return true;
                } else {
                    return false;
                }

            } catch (std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 验证好友关系出错," << exception.what());
                return false;
            }
        }

        std::shared_ptr<pqxx::connection> RosterModule::getDBConnection() {
            if (this->m_dbConnection) {
                return this->m_dbConnection;
            }

            const std::string postgrelConnectionUrl =
                    "dbname=" + this->dbConfig.getDBName() + " user=" + this->dbConfig.getUserAccount() + " password=" +
                    this->dbConfig.getUserPassword() + " hostaddr=" + this->dbConfig.getHostAddr() + " port=" +
                    std::to_string(this->dbConfig.getPort());

            try {

                this->m_dbConnection = std::make_shared<pqxx::connection>(postgrelConnectionUrl);

                if (!this->m_dbConnection->is_open()) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << "打开数据库失败");
                }


            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "连接数据库出错," << exception.what());
            }

            return this->m_dbConnection;
        }
    }
}
