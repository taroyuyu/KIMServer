//
// Created by Kakawater on 2018/3/8.
//

#ifndef KAKAIMCLUSTER_GROUPCHATMESSAGEDELEGATE_H
#define KAKAIMCLUSTER_GROUPCHATMESSAGEDELEGATE_H

namespace kakaIM {
    namespace client {
        class GroupChatMessage;
        class GroupChatMessageDelegate{
        public:
            ~GroupChatMessageDelegate(){

            }
            virtual void didReceiveGroupChatMessage(const GroupChatMessage & groupChatMessage) = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_GROUPCHATMESSAGEDELEGATE_H

