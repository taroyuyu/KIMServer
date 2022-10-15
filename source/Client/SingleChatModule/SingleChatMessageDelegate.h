//
// Created by Kakawater on 2018/2/28.
//

#ifndef KAKAIMCLUSTER_SINGLECAHTMODULEDELEGATE_H
#define KAKAIMCLUSTER_SINGLECAHTMODULEDELEGATE_H
namespace kakaIM {
    namespace client {
        class ChatMessage;
        class SingleChatMessageDelegate{
        public:
            virtual ~SingleChatMessageDelegate(){

            }
            virtual void didReceiveSingleChatMessage(const ChatMessage & chatMessage) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_SINGLECAHTMODULEDELEGATE_H

