//
// Created by taroyuyu on 2018/1/7.
//

#include <zconf.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include "RosterModule.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace node {
        RosterModule::RosterModule() : mEpollInstance(-1),messageEventfd(-1),m_dbConnection(nullptr) {
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

            return true;
        }

        void RosterModule::setDBConfig(const common::KIMDBConfig &dbConfig) {
            this->dbConfig = dbConfig;
        }

        void RosterModule::execute() {
            while (this->m_isStarted) {
                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为0.1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   1000);

                if (-1 == happedEventsCount) {
                    LOG4CXX_WARN(this->logger,__FUNCTION__<<" 等待Epil实例上的事件出错，errno ="<<errno);
                }

                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (EPOLLIN & happedEvents[i].events) {
                        if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < ::read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->messageQueue.empty()) {
                                    this->messageQueueMutex.lock();
                                    auto pairIt = std::move(this->messageQueue.front());
                                    this->messageQueue.pop();
                                    this->messageQueueMutex.unlock();

                                    auto messageType = pairIt.first->GetTypeName();
                                    if (messageType ==
                                        kakaIM::Node::BuildingRelationshipRequestMessage::default_instance().GetTypeName()) {
                                        handleBuildingRelationshipRequestMessage(
                                                *(kakaIM::Node::BuildingRelationshipRequestMessage *) pairIt.first.get(),
                                                pairIt.second);
                                    } else if (messageType ==
                                               kakaIM::Node::BuildingRelationshipAnswerMessage::default_instance().GetTypeName()) {
                                        handleBuildingRelationshipAnswerMessage(
                                                *(kakaIM::Node::BuildingRelationshipAnswerMessage *) pairIt.first.get(),
                                                pairIt.second);
                                    } else if (messageType ==
                                               kakaIM::Node::DestroyingRelationshipRequestMessage::default_instance().GetTypeName()) {
                                        handleDestroyingRelationshipRequestMessage(
                                                *(kakaIM::Node::DestroyingRelationshipRequestMessage *) pairIt.first.get(),
                                                pairIt.second);
                                    } else if (messageType ==
                                               kakaIM::Node::FriendListRequestMessage::default_instance().GetTypeName()) {
                                        handleFriendListRequestMessage(
                                                *(kakaIM::Node::FriendListRequestMessage *) pairIt.first.get(),
                                                pairIt.second);
                                    } else if (messageType ==
                                               kakaIM::Node::FetchUserVCardMessage::default_instance().GetTypeName()) {
                                        handleFetchUserVCardMessage(*(kakaIM::Node::FetchUserVCardMessage *) pairIt.first.get(),
                                                                    pairIt.second);
                                    } else if (messageType ==
                                               kakaIM::Node::UpdateUserVCardMessage::default_instance().GetTypeName()) {
                                        handleUpdateUserVCardMessage(*(kakaIM::Node::UpdateUserVCardMessage *) pairIt.first.get(),
                                                                     pairIt.second);
                                    }
                                }

                            } else {
                                LOG4CXX_WARN(this->logger,
                                             typeid(this).name() << "" << __FUNCTION__ << "read(messageEventfd)操作出错，errno ="
                                                                 << errno);
                            }
                        }
                    }
                }
            }
        }

        void RosterModule::handleBuildingRelationshipRequestMessage(
                const kakaIM::Node::BuildingRelationshipRequestMessage &message,
                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
            //1.查询SessionID对应的userAccount
            std::string userAccount = this->mQueryUserAccountWithSessionServicePtr.lock()->queryUserAccountWithSession(
                    message.sessionid());
            //2.比较发起者与userAccount是否相等
            if (userAccount != message.sponsoraccount()) {
                return;
            }

            kakaIM::Node::BuildingRelationshipRequestMessage requestMessage(message);
            uint64_t applicant_id;
            //3.将此申请保存到数据库
            const std::string AddUserRelationApplicationStatement = "AddUserRelationApplication";
            const std::string addUserRelationApplicationSQL = "INSERT INTO user_relation_applications (sponsor, target, state, submission_time, introduction, applicant_id)"
                    "VALUES ($1, $2, 'Pending', now(), $3, DEFAULT)RETURNING applicant_id;";
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

                requestMessage.set_applicant_id(applicant_id);

                //提交事务
                dbWork.commit();

            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "将好友申请插入数据库失败," << exception.what());
            }

            //4.发送此消息
            auto messageSendServicePtr = this->mMessageSendServicePtr.lock();
            if (messageSendServicePtr) {
                messageSendServicePtr->sendMessageToUser(requestMessage.targetaccount(), requestMessage);
            } else {
                LOG4CXX_WARN(this->logger, typeid(this).name() << "" << __FUNCTION__ << "处理失败，由于不存在messageSendService");
            }
        }

        bool
        RosterModule::checkUserRelationApplication(const std::string sponsorAccount, const std::string targetAccount,
                                                   const uint64_t applicantId) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
            static const std::string CheckApplicantIdStatement = "CheckApplicantId";
            static const std::string checkApplicantIdSQL = "SELECT sponsor,target,state FROM user_relation_applications WHERE applicant_id = $1;";

            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 获取数据库连接失败" << errno);
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

        bool RosterModule::updateUserRelationApplicationState(const uint64_t applicantId,
                                                              kakaIM::Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer answer) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
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
            static const std::string updateUserRelationApplicationStateSQL = "UPDATE user_relation_applications SET state = $1 WHERE applicant_id = $2;";


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
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
            //1.查询SessionID对应的userAccount
            std::string userAccount = this->mQueryUserAccountWithSessionServicePtr.lock()->queryUserAccountWithSession(
                    message.sessionid());
            //2.比较发起者与userAccount是否相等
            if (userAccount != message.targetaccount()) {
                return;
            }//3.检查数据库中是否存在此applicationId
            if (false == this->checkUserRelationApplication(message.sponsoraccount(), message.targetaccount(),
                                                            message.applicant_id())) {
                return;;
            }
            //4.设置好友申请状态
            this->updateUserRelationApplicationState(message.applicant_id(), message.answer());
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

                        pqxx::result result = buildRelationShipSQLStatementInvocation(message.sponsoraccount())(
                                message.targetaccount()).exec();

                        //提交事务
                        dbWork.commit();

                        if (1 == result.affected_rows()) {//好友关系建立成功
                            auto messageSendServicePtr = this->mMessageSendServicePtr.lock();
                            if (messageSendServicePtr) {
                                messageSendServicePtr->sendMessageToUser(message.targetaccount(), message);
                                messageSendServicePtr->sendMessageToUser(message.sponsoraccount(), message);
                            } else {

                            }
                        } else {//好友关系建立失败

                        }
                    } catch (const std::exception &exception) {
                        LOG4CXX_ERROR(this->logger,
                                      typeid(this).name() << "" << __FUNCTION__ << "好友关系建立失败," << exception.what());
                    }

                }
                    break;
                case kakaIM::Node::BuildingRelationshipAnswerMessage_BuildingRelationshipAnswer_Reject: {
                    auto messageSendServicePtr = this->mMessageSendServicePtr.lock();
                    if (messageSendServicePtr) {
                        messageSendServicePtr->sendMessageToUser(message.sponsoraccount(), message);
                    } else {

                    }
                }
                    break;
                default: {

                }
                    break;
            }
        }

        void RosterModule::handleDestroyingRelationshipRequestMessage(
                const kakaIM::Node::DestroyingRelationshipRequestMessage &message,
                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
            kakaIM::Node::DestoryingRelationshipResponseMessage responseMessage;
            responseMessage.set_sponsoraccount(message.sponsoraccount());
            responseMessage.set_targetaccount(message.targetaccount());

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
                LOG4CXX_ERROR(this->logger,__FUNCTION__<<" connectionOperationService不存在");
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

                responseMessage.set_response(
                        kakaIM::Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_Success);

                auto messageSendServicePtr = this->mMessageSendServicePtr.lock();
                if (messageSendServicePtr) {
                    messageSendServicePtr->sendMessageToUser(responseMessage.sponsoraccount(), responseMessage);
                    messageSendServicePtr->sendMessageToUser(responseMessage.targetaccount(), responseMessage);
                } else {
                    LOG4CXX_ERROR(this->logger,__FUNCTION__<<" messageSendServicePtr不存在");
                }

            } catch (pqxx::sql_error exception) {//拆除失败
                responseMessage.set_response(
                        kakaIM::Node::DestoryingRelationshipResponseMessage_DestoryingRelationshipResponse_ServerInteralError);
                responseMessage.set_sessionid(message.sessionid());
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseMessage);
                }else{
                    LOG4CXX_ERROR(this->logger,__FUNCTION__<<" connectionOperationService不存在");
                }
            }
        }

        void RosterModule::handleFriendListRequestMessage(const kakaIM::Node::FriendListRequestMessage &message,
                                                          const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
            auto queryUserAccountWithSessionServicePtr = this->mQueryUserAccountWithSessionServicePtr.lock();
            std::string userAccount;
            if (queryUserAccountWithSessionServicePtr) {
                userAccount = queryUserAccountWithSessionServicePtr->queryUserAccountWithSession(message.sessionid());
            } else {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "" << __FUNCTION__
                                                                << "因queryUserAccountWithSessionService不存在,无法处理");
                return;
            }

            kakaIM::Node::FriendListResponseMessage responseMessage;

            const std::string QueryFriendListSQLStatement = "QueryFriendListSQL";
            const std::string queryFriendListSQL = "SELECT sponsor,target FROM user_relationship WHERE sponsor = $1 OR target = $1 ;";

            try {

                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_WARN(this->logger, typeid(this).name() << "" << __FUNCTION__ << " 获取数据库连接失败");
                    //异常处理
                    return;;
                }

                pqxx::work dbWork(*dbConnection);

                auto queryFriendListSQLStatementInvocation = dbWork.prepared(QueryFriendListSQLStatement);
                if (!queryFriendListSQLStatementInvocation.exists()) {
                    dbConnection->prepare(QueryFriendListSQLStatement, queryFriendListSQL);
                }

                pqxx::result result = queryFriendListSQLStatementInvocation(userAccount).exec();

                for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                    for (auto field = row->begin(); field != row->end(); ++field) {

                        if (userAccount != field->c_str()) {

                            auto friendItem = responseMessage.add_friend_();
                            friendItem->set_friendaccount(field->c_str());
                        }
                    }
                }

                responseMessage.set_sessionid(message.sessionid());
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseMessage);
                }
            } catch (std::exception &exception) {//查询失败
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 处理好友列表出错," << exception.what());
            }
        }

        void RosterModule::handleFetchUserVCardMessage(const kakaIM::Node::FetchUserVCardMessage &message,
                                                       const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
            kakaIM::Node::UserVCardResponseMessage responseMessage;
            responseMessage.set_sessionid(message.sessionid());

            const std::string QueryUserVCardSQLStatement = "QueryUserVCardSQL";
            const std::string queryUserVCardSQL = "SELECT nickname,sex,avator,mood FROM user_vcard WHERE account = $1 ;";

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

                responseMessage.set_userid(message.userid());
                responseMessage.set_nickname(userNickName);
                responseMessage.set_gender(userGenderType);
                responseMessage.set_mood(userMood);
                responseMessage.set_avator(userAvator);

                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseMessage);
                }

            } catch (std::exception exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << " 查询用户:" << message.userid() << "的电子名片信息失败,"
                                                  << exception.what());
            }
        }

        void RosterModule::handleUpdateUserVCardMessage(const kakaIM::Node::UpdateUserVCardMessage &message,
                                                        const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
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
            const std::string updateUserVCardSQL = "UPDATE user_vcard SET nickname = $1 , sex = $2 , avator = $3 , mood = $4 WHERE account = $5 ;";

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

            } catch (std::exception exception) {
                LOG4CXX_WARN(this->logger,
                             typeid(this).name() << "" << __FUNCTION__ << "更新用户电子名片失败," << exception.what());
                response.set_state(kakaIM::Node::UpdateUserVCardMessageResponse_UpdateUserVCardStateType_Failure);
            }

            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, response);
            }
        }

        bool RosterModule::checkFriendRelation(const std::string userA, const std::string userB) {
            LOG4CXX_TRACE(this->logger,__FUNCTION__);
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

        void RosterModule::addBuildingRelationshipRequestMessage(
                const kakaIM::Node::BuildingRelationshipRequestMessage &message,
                const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::BuildingRelationshipRequestMessage> buildingRelationshipRequestMessage(
                    new kakaIM::Node::BuildingRelationshipRequestMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(
                    std::move(buildingRelationshipRequestMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void RosterModule::addBuildingRelationshipAnswerMessage(
                const kakaIM::Node::BuildingRelationshipAnswerMessage &message,
                const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::BuildingRelationshipAnswerMessage> buildingRelationshipAnswerMessage(
                    new kakaIM::Node::BuildingRelationshipAnswerMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(buildingRelationshipAnswerMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void RosterModule::addDestroyingRelationshipRequestMessage(
                const kakaIM::Node::DestroyingRelationshipRequestMessage &message,
                const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::DestroyingRelationshipRequestMessage> destroyingRelationshipRequestMessage(
                    new kakaIM::Node::DestroyingRelationshipRequestMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(
                    std::move(destroyingRelationshipRequestMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void RosterModule::addFriendListRequestMessage(const kakaIM::Node::FriendListRequestMessage &message,
                                                       const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::FriendListRequestMessage> friendListRequestMessage(
                    new kakaIM::Node::FriendListRequestMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(friendListRequestMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void RosterModule::addFetchUserVCardMessage(const kakaIM::Node::FetchUserVCardMessage &message,
                                                    const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::FetchUserVCardMessage> fetchUserVCardMessage(
                    new kakaIM::Node::FetchUserVCardMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(fetchUserVCardMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void RosterModule::addUpdateUserVCardMessage(const kakaIM::Node::UpdateUserVCardMessage &message,
                                                     const std::string connectionIdentifier) {
            std::unique_ptr<kakaIM::Node::UpdateUserVCardMessage> updateUserVCardMessage(
                    new kakaIM::Node::UpdateUserVCardMessage(message));
            //添加到队列中
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(updateUserVCardMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void RosterModule::setConnectionOperationService(
                std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr) {
            this->connectionOperationServicePtr = connectionOperationServicePtr;
        }

        void RosterModule::setQueryUserAccountWithSessionService(
                std::weak_ptr<QueryUserAccountWithSession> queryUserAccountWithSessionServicePtr) {
            this->mQueryUserAccountWithSessionServicePtr = queryUserAccountWithSessionServicePtr;
        }

        void RosterModule::setMessageSendService(std::weak_ptr<MessageSendService> messageSendServicePtr) {
            this->mMessageSendServicePtr = messageSendServicePtr;
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
