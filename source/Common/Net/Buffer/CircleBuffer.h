//
// Created by taroyuyu on 2018/1/3.
//

#ifndef KAKAIMCLUSTER_CIRCLEBUFFER_H
#define KAKAIMCLUSTER_CIRCLEBUFFER_H

#include <mutex>
namespace kakaIM{
    namespace net{
        class CircleBuffer
        {
        public:
            CircleBuffer(size_t initialCapacity);
            ~CircleBuffer();
            void append(const void * buffer,const size_t bufferLength) ;

            size_t retrive(const void * buffer,const size_t bufferCapacity);
            /**
             * 查看缓冲区最前面的内容,但不会删除
             * @param buffer
             * @param bufferLength
             * @return
             */
            size_t head(const void * buffer,const size_t bufferLength);
            size_t getUsed();

            bool check();
        private:
            struct Node
            {
                Node():next(nullptr),previous(nullptr){

                }
                struct Node * next;
                struct Node * previous;
#define NodeContentSize 2
                u_int8_t content[NodeContentSize];
            };
            struct Cursor
            {
                struct Node * node;
                size_t offset;
            } mReadCursor,mWriteCursor;
            size_t mCapacity;
            size_t mUsed;
            std::mutex bufferMutex;

            bool checkCircle();

        };

    }
}

#endif //KAKAIMCLUSTER_CIRCLEBUFFER_H

