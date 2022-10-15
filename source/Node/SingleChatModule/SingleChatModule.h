//
// Created by Kakawater on 2018/1/7.
//

#ifndef KAKAIMCLUSTER_SINGLECHATMODULE_H
#define KAKAIMCLUSTER_SINGLECHATMODULE_H

#include <Node/KIMNodeModule/KIMNodeModule.h>

namespace kakaIM {
    namespace node {
        class SingleChatModule : public KIMNodeModule {
        public:
            SingleChatModule();
        protected:
            void dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> &task);
        private:
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

            enum class SaveVideoChatOfferResult {
                DBConnectionNotExit,//数据库连接不存在
                InteralError,//内部错误
                Success,//查询成功
            };

            SaveVideoChatOfferResult
            saveVideoChatOffer(const std::string spnsorAccount, const std::string targetAccount,
                               const std::string sponsorServer, const std::string sponsorSessionId, uint64_t *offerId,
                               std::string *submissionTime);

            enum class UpdateVideoChatOfferResult {
                DBConnectionNotExit,//数据库连接不存在
                Success,//更新成功
                ParameterErrpr,//参数错误
                Failed,//更新失败
            };

            enum class VideoChatOfferState {
                Pending,//待处理
                Cancel,//被取消
                Accept,//允许
                Reject,//拒绝
                NoAnswer,//无响应
            };

            UpdateVideoChatOfferResult
            updateVideoChatOffer(const uint64_t offerId, std::string recipientServerId, std::string recipientSessionId,
                                 const VideoChatOfferState offerState);

            enum class FetchVideoChatOfferResult {
                DBConnectionNotExit,//数据库连接不存在
                InteralError,//内部错误
                RecordNotExist,//记录不存在
                Success//更新成功
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
        };
    }
}


#endif //KAKAIMCLUSTER_SINGLECHATMODULE_H

