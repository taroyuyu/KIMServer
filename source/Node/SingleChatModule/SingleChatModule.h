//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_SINGLECHATMODULE_H
#define KAKAIMCLUSTER_SINGLECHATMODULE_H

#include <queue>
#include <mutex>
#include <memory>
#include "../KIMNodeModule/KIMNodeModule.h"
#include "../../Common/proto/KakaIMMessage.pb.h"
#include "../../Common/KIMDBConfig.h"
#include "../../Common/ConcurrentQueue/ConcurrentLinkedQueue.h"

namespace kakaIM {
    namespace node {
        class SingleChatModule : public KIMNodeModule {
        public:
            SingleChatModule();

            ~SingleChatModule();

            virtual bool init() override;

            virtual void addMessage(std::unique_ptr<::google::protobuf::Message> message, const std::string connectionIdentifier)override;

        protected:
            virtual void execute() override;

            virtual void shouldStop() override;

            std::atomic_bool m_needStop;
        protected:
            int mEpollInstance;
            int messageEventfd;
            std::mutex messageQueueMutex;
            std::queue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> messageQueue;
            ConcurrentLinkedQueue<std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string>> mTaskQueue;

            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> &task);

            void handleChatMessage(kakaIM::Node::ChatMessage &chatMessage, const std::string connectionIdentifier);

            void handleVideoChatRequestMessage(kakaIM::Node::VideoChatRequestMessage &videoChatRequestMessage,
                                               const std::string connectionIdentifier);

            void handleVideoChatRequestCancelMessage(
                    kakaIM::Node::VideoChatRequestCancelMessage &videoChatRequestCancelMessage,
                    const std::string connectionIdentifier);

            void handleVideoChatReplyMessage(kakaIM::Node::VideoChatReplyMessage &videoChatReplyMessage,
                                             const std::string connectionIdentifier);

            void handleVideoChatOfferMessage(kakaIM::Node::VideoChatOfferMessage &videoChatOfferMessage,
                                             const std::string connectionIdentifier);

            void handleVideoChatAnswerMessage(kakaIM::Node::VideoChatAnswerMessage &videoChatAnswerMessage,
                                              const std::string connectionIdentifier);

            void handleVideoChatNegotiationResultMessage(
                    kakaIM::Node::VideoChatNegotiationResultMessage &videoChatNegotiationResultMessage,
                    const std::string connectionIdentifier);

            void handleVideoChatCandidateAddressMessage(
                    kakaIM::Node::VideoChatCandidateAddressMessage &videoChatCandidateAddressMessage,
                    const std::string connectionIdentifier);

            void handleVideoChatByeMessage(kakaIM::Node::VideoChatByeMessage &videoChatByeMessage,
                                           const std::string connectionIdentifier);

            enum SaveVideoChatOfferResult {
                SaveVideoChatOfferResult_DBConnectionNotExit,//数据库连接不存在
                SaveVideoChatOfferResult_InteralError,//内部错误
                SaveVideoChatOfferResult_Success,//查询成功
            };

            SaveVideoChatOfferResult
            saveVideoChatOffer(const std::string spnsorAccount, const std::string targetAccount,
                               const std::string sponsorServer, const std::string sponsorSessionId, uint64_t *offerId,
                               std::string *submissionTime);

            enum UpdateVideoChatOfferResult {
                UpdateVideoChatOfferResult_DBConnectionNotExit,//数据库连接不存在
                UpdateVideoChatOfferResult_Success,//更新成功
                UpdateVideoChatOfferResult_ParameterErrpr,//参数错误
                UpdateVideoChatOfferResult_Failed,//更新失败
            };

            enum VideoChatOfferState {
                VideoChatOfferState_Pending,//待处理
                VideoChatOfferState_Cancel,//被取消
                VideoChatOfferState_Accept,//允许
                VideoChatOfferState_Reject,//拒绝
                VideoChatOfferState_NoAnswer,//无响应
            };

            UpdateVideoChatOfferResult
            updateVideoChatOffer(const uint64_t offerId, std::string recipientServerId, std::string recipientSessionId,
                                 const VideoChatOfferState offerState);

            enum FetchVideoChatOfferResult {
                FetchVideoChatOfferResult_DBConnectionNotExit,//数据库连接不存在
                FetchVideoChatOfferResult_InteralError,//内部错误
                FetchVideoChatOfferResult_RecordNotExist,//记录不存在
                FetchVideoChatOfferResult_Success//更新成功
            };

            struct VideoChatOfferInfo {
                uint64_t offerId;
                std::string sponsorAccount;
                std::string targetAccount;
                std::string sponsorServerId;
                std::string sponsorSessionId;
                std::string recipientServerId;
                std::string recipientSessionId;
                VideoChatOfferState state;
                std::string submissionTime;
            };

            FetchVideoChatOfferResult fetchVideoChatOffer(const uint64_t offerId, VideoChatOfferInfo &offerInfo);

            std::shared_ptr<pqxx::connection> getDBConnection();
        };
    }
}


#endif //KAKAIMCLUSTER_SINGLECHATMODULE_H

