//
// Created by Kakawater on 2018/1/24.
//

#include <zconf.h>
#include <Node/GroupChatModule/GroupChatModule.h>
#include <Common/proto/MessageCluster.pb.h>
#include <Node/Log/log.h>
#include <Common/util/Date.h>
#include <Common/proto/KakaIMMessage.pb.h>

namespace kakaIM {
    namespace node {

        GroupChatModule::GroupChatModule() : KIMNodeModule(GroupChatModuleLogger){
            this->mMessageTypeSet.insert(kakaIM::Node::ChatGroupCreateRequest::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::ChatGroupDisbandRequest::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::ChatGroupJoinRequest::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::ChatGroupJoinResponse::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::ChatGroupQuitRequest::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::UpdateChatGroupInfoRequest::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::FetchChatGroupInfoRequest::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::FetchChatGroupMemberListRequest::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::FetchChatGroupListRequest::default_instance().GetTypeName());
            this->mMessageTypeSet.insert(kakaIM::Node::GroupChatMessage::default_instance().GetTypeName());
        }

        const std::unordered_set<std::string> & GroupChatModule::messageTypes(){
            return this->mMessageTypeSet;
        }

        void GroupChatModule::dispatchMessage(
                std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> &task) {
            if (task.first->GetTypeName() ==
                kakaIM::Node::ChatGroupCreateRequest::default_instance().GetTypeName()) {
                this->handleChatGroupCreateRequestMessage(
                        *(kakaIM::Node::ChatGroupCreateRequest *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::ChatGroupDisbandRequest::default_instance().GetTypeName()) {
                this->handleChatGroupDisbandRequestMessage(
                        *(kakaIM::Node::ChatGroupDisbandRequest *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::ChatGroupJoinRequest::default_instance().GetTypeName()) {
                this->handleChatGroupJoinRequestMessage(
                        *(kakaIM::Node::ChatGroupJoinRequest *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::ChatGroupJoinResponse::default_instance().GetTypeName()) {
                this->handleChatGroupJoinResponseMessage(
                        *(kakaIM::Node::ChatGroupJoinResponse *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::ChatGroupQuitRequest::default_instance().GetTypeName()) {
                this->handleChatGroupQuitRequestMessage(
                        *(kakaIM::Node::ChatGroupQuitRequest *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::UpdateChatGroupInfoRequest::default_instance().GetTypeName()) {
                this->handleUpdateChatGroupInfoRequestMessage(
                        *(kakaIM::Node::UpdateChatGroupInfoRequest *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::FetchChatGroupInfoRequest::default_instance().GetTypeName()) {
                this->handleFetchChatGroupInfoRequestMessage(
                        *(kakaIM::Node::FetchChatGroupInfoRequest *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::FetchChatGroupMemberListRequest::default_instance().GetTypeName()) {
                this->handleFetchChatGroupMemberListRequestMessage(
                        *(kakaIM::Node::FetchChatGroupMemberListRequest *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::FetchChatGroupListRequest::default_instance().GetTypeName()) {
                this->handleFetchChatGroupListRequestMessage(
                        *(kakaIM::Node::FetchChatGroupListRequest *) task.first.get(),
                        task.second);
            } else if (task.first->GetTypeName() ==
                       kakaIM::Node::GroupChatMessage::default_instance().GetTypeName()) {
                this->handleGroupChatMessage(
                        *(kakaIM::Node::GroupChatMessage *) task.first.get(),
                        task.second);
            }
        }

        void GroupChatModule::handleChatGroupCreateRequestMessage(const kakaIM::Node::ChatGroupCreateRequest &message,
                                                                  const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            std::string groupName = message.groupname();
            std::string groupDescription = message.groupdescrption();
            kakaIM::Node::ChatGroupCreateResponse chatGroupCreateResponse;
            chatGroupCreateResponse.set_sessionid(message.sessionid());
            if (message.has_sign()) {
                chatGroupCreateResponse.set_sign(message.sign());
            }
            if (auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock()) {
                std::string userAccount = queryUserAccountService->queryUserAccountWithSession(message.sessionid());
                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_ERROR(this->logger,
                                  typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败" << errno);
                    //异常处理
                    chatGroupCreateResponse.set_result(
                            kakaIM::Node::ChatGroupCreateResponse_ChatGroupCreateResponseResult_Failed);
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                 chatGroupCreateResponse);
                    }
                    return;
                }

                const std::string CreateGroupSQLStatement = "CreateGroupSQL";
                const std::string createGroupSQL = "INSERT INTO group_info (group_id,group_name, group_descrption, group_master) "
                                                   "VALUES (DEFAULT,$1,$2,$3) RETURNING group_id;";
                const std::string AddGroupManagerSQLStatement = "AddGroupManagerSQL";
                const std::string addGroupManagerSQL = "INSERT INTO group_manager (group_id,userAccount) VALUES($1,$2);";
                const std::string AddGroupMemberSQLStatement = "AddGroupMemberSQL";
                const std::string addGroupMemberSQL = "INSERT INTO group_member (user_account,groupid) "
                                                      "VALUES ( $1,$2);";

                try {
                    //开启事务
                    pqxx::work dbWork(*dbConnection);

                    auto createGroupSQLStatementInvocation = dbWork.prepared(CreateGroupSQLStatement);

                    if (!createGroupSQLStatementInvocation.exists()) {
                        dbConnection->prepare(CreateGroupSQLStatement, createGroupSQL);
                    }

                    //执行
                    pqxx::result r = createGroupSQLStatementInvocation(groupName)(groupDescription)(userAccount).exec();

                    std::string groupId = r[0][0].as<std::string>();

                    auto addGroupManagerSQLStatementInvocation = dbWork.prepared(AddGroupManagerSQLStatement);
                    if (!addGroupManagerSQLStatementInvocation.exists()) {
                        dbConnection->prepare(AddGroupManagerSQLStatement, addGroupManagerSQL);
                    }

                    r = addGroupManagerSQLStatementInvocation(groupId)(userAccount).exec();

                    auto addGroupMemberSQLStatementInvocation = dbWork.prepared(AddGroupMemberSQLStatement);
                    if (!addGroupMemberSQLStatementInvocation.exists()) {
                        dbConnection->prepare(AddGroupMemberSQLStatement, addGroupMemberSQL);
                    }

                    r = addGroupMemberSQLStatementInvocation(userAccount)(groupId).exec();


                    //提交
                    dbWork.commit();
                    chatGroupCreateResponse.set_groupid(groupId);
                    chatGroupCreateResponse.set_result(
                            kakaIM::Node::ChatGroupCreateResponse_ChatGroupCreateResponseResult_Success);

                } catch (const std::exception &e) {
                    LOG4CXX_ERROR(this->logger,
                                  typeid(this).name() << "::" << __FUNCTION__ << " 创建聊天群失败," << e.what());
                    chatGroupCreateResponse.set_result(
                            kakaIM::Node::ChatGroupCreateResponse_ChatGroupCreateResponseResult_Failed);
                }
            } else {
                chatGroupCreateResponse.set_result(
                        kakaIM::Node::ChatGroupCreateResponse_ChatGroupCreateResponseResult_Failed);
            }

            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, chatGroupCreateResponse);
            }
        }

        void GroupChatModule::handleChatGroupDisbandRequestMessage(const kakaIM::Node::ChatGroupDisbandRequest &message,
                                                                   const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            std::string groupId = message.groupid();
            std::string operatorId = message.operatorid();
            kakaIM::Node::ChatGroupDisbandResponse chatGroupDisbandResponse;
            chatGroupDisbandResponse.set_sessionid(message.sessionid());
            chatGroupDisbandResponse.set_groupid(groupId);
            chatGroupDisbandResponse.set_operatorid(operatorId);
            if (message.has_sign()) {
                chatGroupDisbandResponse.set_sign(message.sign());
            }
            if (auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock()) {
                std::string userAccount = queryUserAccountService->queryUserAccountWithSession(message.sessionid());
                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_ERROR(this->logger,
                                  typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败" << errno);
                    //异常处理
                    chatGroupDisbandResponse.set_result(
                            kakaIM::Node::ChatGroupDisbandResponse_ChatGroupDisbandResponseResult_Failed);
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                 chatGroupDisbandResponse);
                    }
                    return;
                }

                const std::string RemoveAllGroupMemberSQLStatement = "RemoveAllGroupMemberSQL";
                const std::string removeAllGroupMemberSQL = "DELETE FROM group_member WHERE groupid = $1 ;";
                const std::string DeleteGroupSQLStatement = "DeleteGroupSQL";
                const std::string deleteGroupSQL = "DELETE FROM group_info WHERE group_id = $1 ;";
                const std::string ClearGroupChatRecordSQLStatement = "ClearGroupChatRecordSQL";
                const std::string clearGroupChatRecordSQL = "DELETE FROM group_chat_timeline WHERE group_id = $1;";

                try {
                    //开启事务
                    pqxx::work dbWork(*dbConnection);

                    auto removeAllGroupMemberSQLStatementInvocation = dbWork.prepared(RemoveAllGroupMemberSQLStatement);

                    if (!removeAllGroupMemberSQLStatementInvocation.exists()) {
                        dbConnection->prepare(RemoveAllGroupMemberSQLStatement, removeAllGroupMemberSQL);
                    }

                    pqxx::result result = removeAllGroupMemberSQLStatementInvocation(groupId).exec();

                    auto clearGroupChatRecordSQLStatementInvocation = dbWork.prepared(ClearGroupChatRecordSQLStatement);

                    if (!clearGroupChatRecordSQLStatementInvocation.exists()) {
                        dbConnection->prepare(ClearGroupChatRecordSQLStatement, clearGroupChatRecordSQL);
                    }

                    result = clearGroupChatRecordSQLStatementInvocation(groupId).exec();

                    auto deleteGroupSQLStatementInvocation = dbWork.prepared(DeleteGroupSQLStatement);

                    if (!deleteGroupSQLStatementInvocation.exists()) {
                        dbConnection->prepare(DeleteGroupSQLStatement, deleteGroupSQL);
                    }

                    result = deleteGroupSQLStatementInvocation(groupId).exec();

                    //提交
                    dbWork.commit();
                    chatGroupDisbandResponse.set_result(
                            kakaIM::Node::ChatGroupDisbandResponse_ChatGroupDisbandResponseResult_Success);

                } catch (const std::exception &e) {
                    LOG4CXX_ERROR(this->logger,
                                  typeid(this).name() << "::" << __FUNCTION__ << " 解散聊天群失败," << e.what());
                    chatGroupDisbandResponse.set_result(
                            kakaIM::Node::ChatGroupDisbandResponse_ChatGroupDisbandResponseResult_Failed);
                }

            } else {
                chatGroupDisbandResponse.set_result(
                        kakaIM::Node::ChatGroupDisbandResponse_ChatGroupDisbandResponseResult_Failed);
            }
            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                         chatGroupDisbandResponse);
            }

        }

        std::pair<bool, uint64_t>
        GroupChatModule::persistChatGroupJoinApplication(const std::string applicant, const std::string group_id,
                                                         const std::string introduction, std::string &submissionTime) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            static const std::string PersistChatGroupJoinApplicationStatement = "PersistChatGroupJoinApplication";
            static const std::string persistChatGroupJoinApplicationSQL = "INSERT INTO group_join_applications (applicant,group_id,introduction,state,submission_time)"
                                                                          "VALUES ($1,$2,$3,'Pending',now()) RETURNING applicant_id,submission_time;";
            uint64_t applicant_id = 0;
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << " 获取数据库连接失败");
                //异常处理
                return std::make_pair(false, 0);
            }

            try {

                pqxx::work dbWork(*dbConnection);

                auto persistChatGroupJoinApplicationStatementInvocation = dbWork.prepared(
                        PersistChatGroupJoinApplicationStatement);

                if (!persistChatGroupJoinApplicationStatementInvocation.exists()) {
                    dbConnection->prepare(PersistChatGroupJoinApplicationStatement, persistChatGroupJoinApplicationSQL);
                }

                pqxx::result result = persistChatGroupJoinApplicationStatementInvocation(applicant)(group_id)(
                        introduction).exec();

                if (1 != result.size()) {
                    return std::make_pair(false, 0);
                }

                auto record = result[0];
                applicant_id = record[record.column_number("applicant_id")].as<uint64_t>();
                submissionTime = record[record.column_number("submission_time")].as<std::string>();

                //提交事务
                dbWork.commit();

            } catch (const std::exception &exception) {
                LOG4CXX_WARN(this->logger,
                             typeid(this).name() << "::" << __FUNCTION__ << " 将聊天群加入申请保存到数据库失败，"
                                                 << exception.what());
                return std::make_pair(false, 0);
            }

            return std::make_pair(true, applicant_id);

        }

        std::list<std::string> GroupChatModule::queryChatGrouoManagerList(const std::string groupId) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            static const std::string QueryChatGroupManagerStatement = "QueryChatGroupManager";
            static const std::string queryChatGroupManagerSQL = "SELECT useraccount FROM group_manager WHERE group_id = $1;";
            std::list<std::string> managerlist;
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败" << errno);
                //异常处理
                return managerlist;
            }

            try {

                pqxx::work dbWork(*dbConnection);

                auto queryChatGroupManagerStatementInvocation = dbWork.prepared(QueryChatGroupManagerStatement);

                if (!queryChatGroupManagerStatementInvocation.exists()) {
                    dbConnection->prepare(QueryChatGroupManagerStatement, queryChatGroupManagerSQL);
                }

                pqxx::result result = queryChatGroupManagerStatementInvocation(groupId).exec();

                //提交事务
                dbWork.commit();

                for (auto row = result.begin(); row != result.end(); ++row) {
                    managerlist.emplace_back(row[row.column_number("useraccount")].as<std::string>());
                }

            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << " 查询聊天群的管理员列表失败,"
                                                  << exception.what());
            }

            return managerlist;
        }

        bool GroupChatModule::validateChatGroupManager(const std::string groupID, const std::string userAccount) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            const std::string ValidateChatGroupManagerStatement = "ValidateChatGroupManager";
            const std::string validateChatGroupManagerSQL = "SELECT useraccount FROM group_manager WHERE group_id = $1 AND useraccount = $2;";

            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                //异常处理
                return false;
            }

            bool flag = false;

            try {

                pqxx::work dbWork(*dbConnection);

                auto validateChatGroupManagerStatementInvocation = dbWork.prepared(ValidateChatGroupManagerStatement);

                if (!validateChatGroupManagerStatementInvocation.exists()) {
                    dbConnection->prepare(ValidateChatGroupManagerStatement, validateChatGroupManagerSQL);
                }

                pqxx::result result = validateChatGroupManagerStatementInvocation(groupID)(userAccount).exec();

                //提交事务
                dbWork.commit();

                flag = (1 == result.size());

            } catch (const std::exception &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << "验证聊天群管理员权限失败，"
                                                  << exception.what());
            }

            return flag;
        }

        bool GroupChatModule::addUserToCharGroup(const std::string userAccount, const std::string groupID) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                //异常处理
                return false;
            }

            bool flag = false;
            const std::string AddGroupMemberSQLStatement = "AddGroupMemberSQL";
            const std::string addGroupMemberSQL = "INSERT INTO group_member (user_account,groupid) "
                                                  "VALUES ($1,$2);";

            try {
                //开启事务
                pqxx::work dbWork(*dbConnection);

                auto addGroupMemberSQLStatementInvocation = dbWork.prepared(AddGroupMemberSQLStatement);
                if (!addGroupMemberSQLStatementInvocation.exists()) {
                    dbConnection->prepare(AddGroupMemberSQLStatement, addGroupMemberSQL);
                }

                pqxx::result result = addGroupMemberSQLStatementInvocation(userAccount)(groupID).exec();

                //提交
                dbWork.commit();

                if (1 == result.affected_rows()) {
                    flag = true;
                }

            } catch (const std::exception &e) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << " 将用户加入聊天群失败," << e.what());
            }

            return flag;
        }

        void GroupChatModule::handleChatGroupJoinRequestMessage(const kakaIM::Node::ChatGroupJoinRequest &message,
                                                                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            const std::string groupId = message.groupid();
            kakaIM::Node::ChatGroupJoinResponse chatGroupJoinResponse;
            chatGroupJoinResponse.set_sessionid(message.sessionid());
            chatGroupJoinResponse.set_useraccount(message.useraccount());
            chatGroupJoinResponse.set_groupid(groupId);
            std::cout << __FUNCTION__ << std::endl;

            if (auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock()) {
                const std::string userAccount = queryUserAccountService->queryUserAccountWithSession(
                        message.sessionid());
                if (message.has_operatorid()) {//由管理员提出申请
                    std::cout << __FUNCTION__ << "由管理员提出申请" << std::endl;

                    chatGroupJoinResponse.set_operatorid(message.operatorid());

                    //1.验证用户
                    const std::string operatorAccount = message.operatorid();
                    if (!(!userAccount.empty() && userAccount == operatorAccount)) {//假冒
                        chatGroupJoinResponse.set_result(
                                kakaIM::Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_InfomationNotMatch);
                        if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                            connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                     chatGroupJoinResponse);
                        }
                        return;
                    }

                    //2.检验用户是否具备管理员权限
                    std::cout << __FUNCTION__ << "operatorAccount=" << operatorAccount << std::endl;
                    if (false == this->validateChatGroupManager(groupId, operatorAccount)) {//不具备管理员工权限
                        chatGroupJoinResponse.set_result(
                                kakaIM::Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_AuthorizationNotMath);
                        if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                            connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                     chatGroupJoinResponse);
                        }
                        return;
                    }

                    //3.将用户加入聊天群
                    std::cout << "将" << message.useraccount() << "加入聊天群" << std::endl;
                    const std::string targetUserAccount = message.useraccount();
                    if (this->addUserToCharGroup(targetUserAccount, groupId)) {//加入成功
                        chatGroupJoinResponse.set_result(
                                kakaIM::Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_Allow);
                    } else {//加入失败
                        chatGroupJoinResponse.set_result(
                                kakaIM::Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_Reject);
                    }
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                 chatGroupJoinResponse);
                    }
                    return;
                } else {//用户主动申请加入
                    if (userAccount != message.useraccount()) {
                        chatGroupJoinResponse.set_result(
                                kakaIM::Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_InfomationNotMatch);
                        if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                            connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                     chatGroupJoinResponse);
                        }
                        return;
                    }

                    //1.将此申请记录保存到数据库
                    std::string submissionTime;
                    auto resultPair = this->persistChatGroupJoinApplication(userAccount, groupId,
                                                                            message.introduction(), submissionTime);
                    submissionTime = kaka::Date(submissionTime).toString();

                    if (!resultPair.first) {
                        //错误处理
                        chatGroupJoinResponse.set_result(
                                kakaIM::Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_InfomationNotMatch);
                        if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                            connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                     chatGroupJoinResponse);
                        }
                        return;
                    }
                    //2.将此申请转发给此聊天群的管理员,若在线的话
                    //2.1查询此聊天群的管理员
                    auto managerList = this->queryChatGrouoManagerList(groupId);
                    //2.2将申请入群的消息发送给管理员
                    auto messageSendService = this->mMessageSendServicePtr.lock();
                    if (!messageSendService) {
                        return;
                    }
                    kakaIM::Node::ChatGroupJoinRequest joinRequest(message);
                    joinRequest.set_submissiontime(submissionTime);
                    joinRequest.set_applicant_id(resultPair.second);
                    if (message.has_introduction()) {
                        joinRequest.set_introduction(message.introduction());
                    }
                    for (auto managerAccount: managerList) {
                        messageSendService->sendMessageToUser(managerAccount, joinRequest);
                    }
                    //2.3将申请发送给发送端
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier, joinRequest);
                    }
                    return;
                }
            } else {
                chatGroupJoinResponse.set_result(
                        kakaIM::Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_ServerInternalError);//服务器内部错误
                if (message.has_operatorid()) {
                    chatGroupJoinResponse.set_operatorid(message.operatorid());
                }

                if (message.has_applicant_id()) {
                    chatGroupJoinResponse.set_applicant_id(message.applicant_id());
                }
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                             chatGroupJoinResponse);
                }
            }
        }

        std::pair<bool, struct GroupChatModule::GroupJoinApplication>
        GroupChatModule::fetchGroupJoinApplicationInfo(const uint64_t applicantId) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            const std::string FetchGroupJoinApplicationInfoStatement = "FetchGroupJoinApplicationInfo";
            const std::string fetchGroupJoinApplicationInfoSQL = "SELECT applicant,group_id,state FROM group_join_applications WHERE applicant_id = $1;";
            GroupJoinApplication application;
            application.applicant_id = 0;
            application.group_id = 0;
            bool flag = false;
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                //异常处理
                return std::make_pair(flag, application);
            }

            try {
                //开启事务
                pqxx::work dbWork(*dbConnection);

                auto fetchGroupJoinApplicationInfoStatementInvocation = dbWork.prepared(
                        FetchGroupJoinApplicationInfoStatement);
                if (!fetchGroupJoinApplicationInfoStatementInvocation.exists()) {
                    dbConnection->prepare(FetchGroupJoinApplicationInfoStatement, fetchGroupJoinApplicationInfoSQL);
                }

                pqxx::result result = fetchGroupJoinApplicationInfoStatementInvocation(applicantId).exec();

                //提交
                dbWork.commit();

                if (1 == result.size()) {
                    auto row = result[0];
                    application.applicant_id = applicantId;
                    application.applicant = row[row.column_number("applicant")].as<std::string>();
                    application.group_id = row[row.column_number("group_id")].as<uint64_t>();
                    const std::string applicationState = row[row.column_number("state")].as<std::string>();
                    if (applicationState == "Pending") {
                        application.state = GroupJoinApplicationState::Pending;
                    } else if (applicationState == "Allow") {
                        application.state = GroupJoinApplicationState::Allow;
                    } else if (applicationState == "Reject") {
                        application.state = GroupJoinApplicationState::Reject;
                    }
                    flag = true;
                }

            } catch (const std::exception &e) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << "查询聊天群加入申请失败," << e.what());
            }

            return std::make_pair(flag, application);
        }

        bool GroupChatModule::updateGroupJoinApplicationState(const uint64_t applicantId,
                                                              enum GroupJoinApplicationState state) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            const std::string UpdateGroupJoinApplicationStateStatement = "UpdateGroupJoinApplicationState";
            const std::string updateGroupJoinApplicationStateSQL = "UPDATE group_join_applications SET state = $1  WHERE applicant_id = $2;";

            std::string applicationState;

            switch (state) {
                case GroupJoinApplicationState::Pending: {
                    applicationState = "Pending";
                }
                    break;
                case GroupJoinApplicationState::Allow: {
                    applicationState = "Allow";
                }
                    break;
                case GroupJoinApplicationState::Reject: {
                    applicationState = "Reject";
                }
                    break;
                default: {
                    return false;
                }
            }

            bool flag = false;
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                //异常处理
                return false;
            }

            try {
                //开启事务
                pqxx::work dbWork(*dbConnection);

                auto updateGroupJoinApplicationStateStatementInvocation = dbWork.prepared(
                        UpdateGroupJoinApplicationStateStatement);

                if (!updateGroupJoinApplicationStateStatementInvocation.exists()) {
                    dbConnection->prepare(UpdateGroupJoinApplicationStateStatement, updateGroupJoinApplicationStateSQL);
                }

                pqxx::result result = updateGroupJoinApplicationStateStatementInvocation(applicationState)(
                        applicantId).exec();

                if (1 == result.affected_rows()) {
                    dbWork.commit();
                    flag = true;
                }
            } catch (const std::exception &e) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << " 更新聊天群加入申请的状态失败,"
                                                  << e.what());
            }

            return flag;
        }

        void GroupChatModule::handleChatGroupJoinResponseMessage(const kakaIM::Node::ChatGroupJoinResponse &message,
                                                                 const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            if (message.has_operatorid() && message.has_applicant_id()) {
                return;
            }
            auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock();

            if (!queryUserAccountService) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__
                                                  << " 执行失败，由于queryUserAccountService不存在");
                return;;
            }

            const std::string operatorid = message.operatorid();

            //1.验证用户
            if (!(!operatorid.empty() &&
                  operatorid == queryUserAccountService->queryUserAccountWithSession(message.sessionid()))) {
                return;
            }

            //2.获取此条申请记录的信息
            const std::string applicant = message.useraccount();
            const uint64_t applicant_id = message.applicant_id();
            const std::string groupId = message.groupid();
            auto recordPair = this->fetchGroupJoinApplicationInfo(applicant_id);
            if (false == recordPair.first) {
                return;
            }
            //3.与申请记录的信息进行对比
            if (!(recordPair.second.applicant_id == applicant_id && recordPair.second.applicant == applicant &&
                  std::to_string(recordPair.second.group_id) == groupId)) {//不一致
                return;;
            }

            if (recordPair.second.state != GroupJoinApplicationState::Pending) {//此申请记录已经受理
                return;
            }

            //4.对申请进行处理
            if (message.result() == Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_Allow) {//允许将此用户加入聊天群
                //将用户加入聊天群
                if (this->addUserToCharGroup(applicant, groupId)) {
                    //更新此申请记录的状态
                    this->updateGroupJoinApplicationState(applicant_id, GroupJoinApplicationState::Allow);
                }
            } else if (message.result() ==
                       Node::ChatGroupJoinResponse_ChatGroupJoinResponseResult_Reject) {//拒绝将此用户加入聊天群
                //更新此申请记录的状态
                this->updateGroupJoinApplicationState(applicant_id, GroupJoinApplicationState::Reject);
            }

            //5.通知申请者
            if (auto messageSendService = this->mMessageSendServicePtr.lock()) {
                messageSendService->sendMessageToUser(applicant, message);
            }
        }

        void GroupChatModule::handleChatGroupQuitRequestMessage(const kakaIM::Node::ChatGroupQuitRequest &message,
                                                                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            const std::string groupId = message.groupid();
            kakaIM::Node::ChatGroupQuitResponse chatGroupQuitResponse;
            chatGroupQuitResponse.set_sessionid(message.sessionid());
            chatGroupQuitResponse.set_useraccount(message.useraccount());
            chatGroupQuitResponse.set_groupid(groupId);
            if (message.has_sign()) {
                chatGroupQuitResponse.set_sign(message.sign());
            }
            if (auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock()) {
                const std::string userAccount = queryUserAccountService->queryUserAccountWithSession(
                        message.sessionid());

                if (message.has_operatorid()) {
                    chatGroupQuitResponse.set_operatorid(message.operatorid());
                    const std::string operatorAccount = message.operatorid();
                    if (userAccount != operatorAccount) {
                        chatGroupQuitResponse.set_result(
                                kakaIM::Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Failed);
                    }

                    //暂时不支持此功能
                    chatGroupQuitResponse.set_result(
                            kakaIM::Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Failed);

                } else {//用户主动申请退出

                    if (userAccount != message.useraccount()) {
                        chatGroupQuitResponse.set_result(
                                kakaIM::Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Failed);
                    }

                    auto dbConnection = this->getDBConnection();
                    if (!dbConnection) {
                        LOG4CXX_ERROR(this->logger,
                                      typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                        //异常处理
                        chatGroupQuitResponse.set_result(
                                kakaIM::Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Failed);
                        if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                            connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                     chatGroupQuitResponse);
                        }
                        return;
                    }

                    const std::string DeleteGroupMemeberSQLStatement = "DeleteGroupMemeberSQL";
                    const std::string deleteGroupMemeberSQL = "DELETE FROM group_member WHERE groupid = $1 AND user_account = $2 ;";

                    try {
                        //开启事务
                        pqxx::work dbWork(*dbConnection);

                        auto deleteGroupMemeberSQLStatementInvocation = dbWork.prepared(DeleteGroupMemeberSQLStatement);

                        if (!deleteGroupMemeberSQLStatementInvocation.exists()) {
                            dbConnection->prepare(DeleteGroupMemeberSQLStatement, deleteGroupMemeberSQL);
                        }

                        pqxx::result result = deleteGroupMemeberSQLStatementInvocation(groupId)(userAccount).exec();

                        //提交
                        dbWork.commit();
                        chatGroupQuitResponse.set_result(
                                kakaIM::Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Success);

                    } catch (const std::exception &e) {
                        LOG4CXX_ERROR(this->logger,
                                      typeid(this).name() << "::" << __FUNCTION__ << "退出聊天群操作失败," << e.what());
                        chatGroupQuitResponse.set_result(
                                kakaIM::Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Failed);
                    }
                }

            } else {
                chatGroupQuitResponse.set_result(
                        kakaIM::Node::ChatGroupQuitResponse_ChatGroupQuitResponseResult_Failed);
            }
            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, chatGroupQuitResponse);
            }
        }

        void GroupChatModule::handleUpdateChatGroupInfoRequestMessage(
                const kakaIM::Node::UpdateChatGroupInfoRequest &message, const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            const std::string groupId = message.groupid();
            const std::string groupName = message.groupname();
            const std::string groupDescription = message.groupdescrption();
            kakaIM::Node::UpdateChatGroupInfoResponse updateChatGroupInfoResponse;
            updateChatGroupInfoResponse.set_sessionid(message.sessionid());
            updateChatGroupInfoResponse.set_groupid(groupId);
            if (message.has_sign()) {
                updateChatGroupInfoResponse.set_sign(message.sign());
            }
            if (auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock()) {
                const std::string operatorAccount = queryUserAccountService->queryUserAccountWithSession(
                        message.sessionid());

                //1.验证operatorAccount是否为管理员

                if (false == this->validateChatGroupManager(groupId, operatorAccount)) {//不是管理员
                    updateChatGroupInfoResponse.set_result(
                            kakaIM::Node::UpdateChatGroupInfoResponse_UpdateChatGroupInfoResponseResult_AuthorizationNotMath);
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                 updateChatGroupInfoResponse);
                    }
                    return;
                }

                //2.执行更新操作

                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                    //异常处理
                    updateChatGroupInfoResponse.set_result(
                            kakaIM::Node::UpdateChatGroupInfoResponse_UpdateChatGroupInfoResponseResult_ServerInternalError);
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                 updateChatGroupInfoResponse);
                    }
                    return;
                }

                const std::string UpdateGroupInfoSQLStatement = "UpdateGroupInfoSQL";
                const std::string updateGroupInfoSQL = "UPDATE group_info SET group_name = $1 , group_descrption = $2 WHERE group_id = $3 ;";

                try {
                    //开启事务
                    pqxx::work dbWork(*dbConnection);

                    auto updateGroupInfoSQLStatementInvocation = dbWork.prepared(UpdateGroupInfoSQLStatement);

                    if (!updateGroupInfoSQLStatementInvocation.exists()) {
                        dbConnection->prepare(UpdateGroupInfoSQLStatement, updateGroupInfoSQL);
                    }

                    pqxx::result result = updateGroupInfoSQLStatementInvocation(groupName)(groupDescription)(
                            groupId).exec();

                    //提交
                    dbWork.commit();
                    updateChatGroupInfoResponse.set_operator_(operatorAccount);
                    updateChatGroupInfoResponse.set_result(
                            kakaIM::Node::UpdateChatGroupInfoResponse_UpdateChatGroupInfoResponseResult_Success);

                } catch (const std::exception &e) {
                    LOG4CXX_ERROR(this->logger,
                                  typeid(this).name() << "::" << __FUNCTION__ << "更新聊天群资料失败," << e.what());
                    updateChatGroupInfoResponse.set_result(
                            kakaIM::Node::UpdateChatGroupInfoResponse_UpdateChatGroupInfoResponseResult_ServerInternalError);
                }


            } else {
                updateChatGroupInfoResponse.set_result(
                        kakaIM::Node::UpdateChatGroupInfoResponse_UpdateChatGroupInfoResponseResult_ServerInternalError);
            }

            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                         updateChatGroupInfoResponse);
            }
        }

        void
        GroupChatModule::handleFetchChatGroupInfoRequestMessage(const kakaIM::Node::FetchChatGroupInfoRequest &message,
                                                                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            const std::string groupId = message.groupid();
            kakaIM::Node::FetchChatGroupInfoResponse fetchChatGroupInfoResponse;
            fetchChatGroupInfoResponse.set_sessionid(message.sessionid());
            fetchChatGroupInfoResponse.set_groupid(groupId);
            if (auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock()) {
                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                    //异常处理
                    fetchChatGroupInfoResponse.set_result(
                            kakaIM::Node::FetchChatGroupInfoResponse_FetchChatGroupInfoResponseResult_Failed);
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                 fetchChatGroupInfoResponse);
                    }
                    return;
                }

                const std::string QueryGroupInfoSQLStatement = "QueryGroupInfoSQL";
                const std::string queryGroupInfoSQL = "SELECT group_name,group_descrption,group_master FROM group_info WHERE group_id = $1 ;";

                try {
                    //开启事务
                    pqxx::work dbWork(*dbConnection);

                    auto queryGroupInfoSQLStatementInvocation = dbWork.prepared(QueryGroupInfoSQLStatement);

                    if (!queryGroupInfoSQLStatementInvocation.exists()) {
                        dbConnection->prepare(QueryGroupInfoSQLStatement, queryGroupInfoSQL);
                    }

                    pqxx::result result = queryGroupInfoSQLStatementInvocation(groupId).exec();

                    if (1 != result.size()) {
                        throw std::exception();
                    }

                    auto row = result.begin();

                    fetchChatGroupInfoResponse.set_groupname(row[0].as<std::string>());
                    fetchChatGroupInfoResponse.set_groupdescrption(row[1].as<std::string>());
                    fetchChatGroupInfoResponse.set_groupmaster(row[2].as<std::string>());

                    //提交
                    dbWork.commit();
                    fetchChatGroupInfoResponse.set_result(
                            kakaIM::Node::FetchChatGroupInfoResponse_FetchChatGroupInfoResponseResult_Success);

                } catch (const std::exception &e) {
                    LOG4CXX_ERROR(this->logger,
                                  typeid(this).name() << "::" << __FUNCTION__ << " 获取聊天群资料失败," << e.what());
                    fetchChatGroupInfoResponse.set_result(
                            kakaIM::Node::FetchChatGroupInfoResponse_FetchChatGroupInfoResponseResult_Failed);
                }

            } else {
                fetchChatGroupInfoResponse.set_result(
                        kakaIM::Node::FetchChatGroupInfoResponse_FetchChatGroupInfoResponseResult_Failed);
            }
            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                         fetchChatGroupInfoResponse);
            }
        }

        void GroupChatModule::handleFetchChatGroupMemberListRequestMessage(
                const kakaIM::Node::FetchChatGroupMemberListRequest &message, const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            const std::string groupId = message.groupid();
            kakaIM::Node::FetchChatGroupMemberListResponse fetchChatGroupMemberListResponse;
            fetchChatGroupMemberListResponse.set_sessionid(message.sessionid());
            fetchChatGroupMemberListResponse.set_groupid(groupId);
            if (message.has_sign()) {
                fetchChatGroupMemberListResponse.set_sign(message.sign());
            }
            //1.判断此用户是否在此群中


            //2.查询
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                //异常处理
                fetchChatGroupMemberListResponse.set_result(
                        kakaIM::Node::FetchChatGroupMemberListResponse_FetchChatGroupMemberListResponseResult_Failed);
                if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                             fetchChatGroupMemberListResponse);
                }
                return;
            }

            const std::string QueryGroupMemberSQLStatement = "QueryGroupMemberSQL";
            const std::string queryGroupMemberSQL = "SELECT user_account,group_nickname FROM group_member WHERE groupid = $1 ;";

            try {
                //开启事务
                pqxx::work dbWork(*dbConnection);

                auto queryGroupMemberSQLStatementInvocation = dbWork.prepared(QueryGroupMemberSQLStatement);

                if (!queryGroupMemberSQLStatementInvocation.exists()) {
                    dbConnection->prepare(QueryGroupMemberSQLStatement, queryGroupMemberSQL);
                }

                pqxx::result result = queryGroupMemberSQLStatementInvocation(groupId).exec();

                for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                    auto item = fetchChatGroupMemberListResponse.add_groupmember();
                    item->set_useraccount(row[0].as<std::string>());
                    if (!row[1].is_null()) {
                        item->set_groupnickname(row[1].as<std::string>());
                    }
                }

                //提交
                dbWork.commit();

                fetchChatGroupMemberListResponse.set_result(
                        kakaIM::Node::FetchChatGroupMemberListResponse_FetchChatGroupMemberListResponseResult_Success);

            } catch (const std::exception &e) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << "获取群成员列表失败," << e.what());
                fetchChatGroupMemberListResponse.set_result(
                        kakaIM::Node::FetchChatGroupMemberListResponse_FetchChatGroupMemberListResponseResult_Failed);
            }

            if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                         fetchChatGroupMemberListResponse);
            }
        }

        void
        GroupChatModule::handleFetchChatGroupListRequestMessage(const kakaIM::Node::FetchChatGroupListRequest &message,
                                                                const std::string connectionIdentifier) {
            LOG4CXX_TRACE(this->logger, __FUNCTION__);
            kakaIM::Node::FetchChatGroupListResponse fetchChatGroupListResponse;
            fetchChatGroupListResponse.set_sessionid(message.sessionid());
            if (auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock()) {
                const std::string userAccount = queryUserAccountService->queryUserAccountWithSession(
                        message.sessionid());

                auto dbConnection = this->getDBConnection();
                if (!dbConnection) {
                    LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                    //异常处理
                    return;
                }
                const std::string QueryGroupListSQLStatement = "QueryGroupListSQL";
                const std::string queryGroupListSQL = "SELECT group_info.group_id , group_info.group_name FROM group_info INNER JOIN group_member ON group_info.group_id = group_member.groupid WHERE  user_account = $1 ;";

                try {
                    //开启事务
                    pqxx::work dbWork(*dbConnection);

                    auto queryGroupListSQLStatementInvocation = dbWork.prepared(QueryGroupListSQLStatement);

                    if (!queryGroupListSQLStatementInvocation.exists()) {
                        dbConnection->prepare(QueryGroupListSQLStatement, queryGroupListSQL);
                    }

                    pqxx::result result = queryGroupListSQLStatementInvocation(userAccount).exec();

                    for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                        auto item = fetchChatGroupListResponse.add_group();
                        item->set_groupid(row[0].as<std::string>());
                        item->set_groupname(row[1].as<std::string>());
                    }

                    //提交
                    dbWork.commit();
                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                        connectionOperationService->sendMessageThroughConnection(connectionIdentifier,
                                                                                 fetchChatGroupListResponse);
                    }

                } catch (const std::exception &e) {
                    LOG4CXX_ERROR(this->logger,
                                  typeid(this).name() << "::" << __FUNCTION__ << " 用户的聊天群列表失败," << e.what());
                }
            }
        }

        void GroupChatModule::handleGroupChatMessage(const kakaIM::Node::GroupChatMessage &groupChatMessage,
                                                     const std::string connectionIdentifier) {
            kakaIM::Node::GroupChatMessage message(groupChatMessage);
            message.set_timestamp(kaka::Date::getCurrentDate().toString());
            const std::string senderDeviceSessionID = message.sessionid();
            auto queryUserAccountService = this->mQueryUserAccountWithSessionServicePtr.lock();
            auto messageIDGenerator = this->mMessageIDGenerateServicePtr.lock();
            auto messagePersister = this->mMessagePersistenceServicePtr.lock();
            auto messageSendService = this->mMessageSendServicePtr.lock();
            if (!(queryUserAccountService && messageIDGenerator && messagePersister && messageSendService)) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__
                                                                << " 无法执行，缺少queryUserAccountService或者messageIDGenerator或者messagePersiste再或者messageSendService");
                return;
            }

            const std::string senderAccount = message.sender();
            const std::string resultAccount = queryUserAccountService->queryUserAccountWithSession(message.sessionid());
            if (senderAccount != resultAccount) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__
                                                                << "用户账号不匹配,senderAccount=" << senderAccount
                                                                << " resultAccount=" << resultAccount);
                return;
            }

            const std::string groupId = message.groupid();
            auto future = messageIDGenerator->generateMessageIDWithUserAccount(groupId);
            uint64_t messageID = future->getMessageID();

            //1.将此消息进行序列化
            messagePersister->persistGroupChatMessage(message, messageID);

            //2.查询此群的成员列表
            auto dbConnection = this->getDBConnection();
            if (!dbConnection) {
                LOG4CXX_ERROR(this->logger, typeid(this).name() << "::" << __FUNCTION__ << "获取数据库连接失败");
                //异常处理
                return;
            }
            std::list<std::string> groupMemeberList;

            const std::string QueryGroupMemberSQLStatement = "QueryGroupMemberSQL";
            const std::string queryGroupMemberSQL = "SELECT user_account FROM group_member WHERE groupid = $1 ;";
            try {
                //开启事务
                pqxx::work dbWork(*dbConnection);

                auto queryGroupMemberSQLStatementInvocation = dbWork.prepared(QueryGroupMemberSQLStatement);

                if (!queryGroupMemberSQLStatementInvocation.exists()) {
                    dbConnection->prepare(QueryGroupMemberSQLStatement, queryGroupMemberSQL);
                }

                pqxx::result result = queryGroupMemberSQLStatementInvocation(groupId).exec();

                for (pqxx::result::const_iterator row = result.begin(); row != result.end(); ++row) {
                    groupMemeberList.emplace_back(row[0].as<std::string>());
                }

                //提交
                dbWork.commit();
            } catch (const std::exception &e) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "::" << __FUNCTION__ << "查询群成员列表失败," << e.what());
            }

            LOG4CXX_TRACE(this->logger, typeid(this).name() << "::" << __FUNCTION__ << " 被调用");
            //3.将此消息发送给在线成员
            auto queryConnectionWithSessionService = this->mQueryConnectionWithSessionServicePtr.lock();
            auto clusterService = this->mClusterServicePtr.lock();
            if (auto loginDeviceQueryService = this->mLoginDeviceQueryServicePtr.lock()) {

                kakaIM::Node::GroupChatMessage groupChatMessage(message);
                groupChatMessage.set_msgid(messageID);

                for (auto groupMember: groupMemeberList) {
                    auto pair = loginDeviceQueryService->queryLoginDeviceSetWithUserAccount(groupMember);
                    for (auto it = pair.first; it != pair.second; ++it) {
                        if (Node::OnlineStateMessage_OnlineState_Offline != it->second) {
                            if (LoginDeviceQueryService::IDType_SessionID == it->first.first) {
                                const std::string sessionID = it->first.second;
                                groupChatMessage.set_sessionid(sessionID);
                                const std::string deviceConnectionIdentifier = queryConnectionWithSessionService->queryConnectionWithSession(
                                        sessionID);
                                if (!deviceConnectionIdentifier.empty()) {
                                    if (auto connectionOperationService = this->connectionOperationServicePtr.lock()) {
                                        connectionOperationService->sendMessageThroughConnection(
                                                deviceConnectionIdentifier, groupChatMessage);
                                    }
                                }
                            } else if (LoginDeviceQueryService::IDType_ServerID == it->first.first) {
                                const std::string serverID = it->first.second;
                                kakaIM::president::ServerMessage serverMessage;
                                serverMessage.set_serverid(serverID);
                                serverMessage.set_messagetype(groupChatMessage.GetTypeName());
                                serverMessage.set_content(groupChatMessage.SerializeAsString());
                                serverMessage.set_targetuser(groupMember);
                                assert(serverMessage.IsInitialized());
                                clusterService->sendServerMessage(serverMessage);
                            }
                        }
                    }
                }
            }

        }

    }
}

