//
// Created by Kakawater on 2018/2/24.
//

#ifndef KAKAIMCLUSTER_BUFFER_H
#define KAKAIMCLUSTER_BUFFER_H
namespace kakaIM {
    namespace net {
        class Buffer{
        public:
            virtual ~Buffer();
            virtual void append(const void * buffer,const size_t bufferLength);
            virtual size_t retrive(const void * buffer,const size_t bufferCapacity) = 0;
            /**
             * 查看缓冲区最前面的内容,但不会删除
             * @param buffer
             * @param bufferLength
             * @return
             */
            virtual size_t head(const void * buffer,const size_t bufferLength) = 0;
            virtual size_t getUsed() = 0;
        };
    }
}
#endif //KAKAIMCLUSTER_BUFFER_H
