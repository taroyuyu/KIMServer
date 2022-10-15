//
// Created by Kakawater on 2018/2/28.
//

#ifndef KAKAIMCLUSTER_SINGLECHATMESSAGEDB_H
#define KAKAIMCLUSTER_SINGLECHATMESSAGEDB_H

namespace kakaIM {
    namespace client {
        class ChatMessage;
        class SingleChatMessageDB{
        public:
            virtual ~SingleChatMessageDB(){

            }
            virtual void storeChatMessage(const ChatMessage & chatMessage) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_SINGLECHATMESSAGEDB_H

