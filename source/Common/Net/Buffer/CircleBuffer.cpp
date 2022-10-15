//
// Created by Kakawater on 2018/1/3.
//

#include <cstring>
#include <iostream>
#include <set>
#include "CircleBuffer.h"
namespace kakaIM{
    namespace net{
        CircleBuffer::CircleBuffer(size_t initialCapacity):
                mCapacity(0),mUsed(0)
        {
            if (initialCapacity <= 0) {
                initialCapacity = NodeContentSize;
            }

            size_t initialNodeCount = (initialCapacity%NodeContentSize == 0) ? (initialCapacity/NodeContentSize) : (initialCapacity/NodeContentSize + 1);
            this->mCapacity = initialNodeCount * NodeContentSize;
            this->mWriteCursor.node = new Node();
            this->mWriteCursor.node->next = this->mWriteCursor.node;
            this->mWriteCursor.node->previous = this->mWriteCursor.node;
            this->mWriteCursor.offset = 0;//指向下一个可写的位置
            this->mReadCursor.node = this->mWriteCursor.node;
            this->mReadCursor.offset = 0;//指向下一个可读的位置

            Node * it = this->mWriteCursor.node;;
            for (size_t i = 1; i < initialNodeCount; ++i) {
                Node * newNode = new Node();
                newNode->next = it->next;
                it->next->previous = newNode;
                it->next = newNode;
                newNode->previous = it;
                it = newNode;
            }

        }

        bool CircleBuffer::checkCircle(){
            Node * it_next = this->mWriteCursor.node;
            Node * it_previois = this->mWriteCursor.node;
            std::set<Node*> nodeSet_a;
            std::set<Node*> nodeSet_b;
            size_t nodeCount = this->mCapacity / NodeContentSize;
            for (size_t i = 0; i < nodeCount; ++i) {
                nodeSet_a.insert(it_next);
                nodeSet_b.insert(it_next);
                it_next = it_next->next;
                it_previois = it_previois->previous;
            }

            return ((it_next == it_previois) && (it_next == this->mWriteCursor.node)) && ((nodeSet_a.size() == nodeCount ) && (nodeSet_b.size() == nodeCount));
        }


        void CircleBuffer::append(const void * buffer,const size_t bufferLength) {
            std::lock_guard<std::mutex> lock(this->bufferMutex);
            const u_int8_t * src = (const u_int8_t *) buffer;

            size_t leftCopyLength = bufferLength;
            while (leftCopyLength > 0) {
                if (this->mWriteCursor.node != this->mReadCursor.node ||
                    this->mWriteCursor.offset >= this->mReadCursor.offset) {//读、写游标不位于同一个节点，或者写指针的游标在读指针的游标的右边(第二个条件的=号是因为CircleBuffer中的ReadCursor和WriteCursor的offset最初是相等的，一旦整个CircleBuffer全部填满，就会新增一个节点(即始终指向下一个可写的位置)，故除了开始的时候，后面ReadCursor和WriteCursor的offset不可能相等)
                    size_t nodeLeftFreeCapaicty = NodeContentSize - this->mWriteCursor.offset;//获取写游标当前所处节点的剩余空间
                    uint8_t * dst = this->mWriteCursor.node->content + this->mWriteCursor.offset;
                    size_t copyLength = std::min(nodeLeftFreeCapaicty,leftCopyLength);
                    memcpy(dst, src, copyLength);//将数据拷贝到写游标所处的当前节点
                    this->mUsed += copyLength;
                    leftCopyLength -= copyLength;
                    nodeLeftFreeCapaicty -=copyLength;
                    src += copyLength;
                    this->mWriteCursor.offset += copyLength;
                    if (0 == nodeLeftFreeCapaicty) {//当前节点的剩余空间已经用完
                        //1.若整个CircleBuffer还存在剩余空间，则跳转到下一个节点
                        if ((this->mCapacity - this->mUsed) > 0) {
                            this->mWriteCursor.node = this->mWriteCursor.node->next;
                            this->mWriteCursor.offset = 0;
                        } else {//否则新增加一个节点
                            Node *newNode = new Node();
                            newNode->next = this->mWriteCursor.node->next;
                            this->mWriteCursor.node->next->previous = newNode;
                            newNode->previous = this->mWriteCursor.node;
                            this->mWriteCursor.node->next = newNode;
                            this->mWriteCursor.node = newNode;
                            this->mWriteCursor.offset = 0;
                            this->mCapacity += NodeContentSize;
                        }
                    }
                } else {//读、写游标位于同一个节点,且写指针的游标在读指针的游标的左边
                    if ((this->mReadCursor.offset - this->mWriteCursor.offset) > leftCopyLength) {//CircleBuffer剩余空间足够存放
                        uint8_t *dst = this->mWriteCursor.node->content + this->mWriteCursor.offset;
                        size_t copyLength = leftCopyLength;
                        memcpy(dst, src, copyLength);//将数据拷贝到写游标所处的当前节点
                        leftCopyLength -= copyLength;
                        src += copyLength;
                        this->mUsed += copyLength;
                        this->mWriteCursor.offset += copyLength;
                    } else {//CircleBuffer剩余空间不足
                        size_t moreSize = leftCopyLength + 1;
                        size_t moreNodeCount = (moreSize % NodeContentSize == 0) ? (moreSize / NodeContentSize) : (
                                moreSize / NodeContentSize + 1);
                        Node *it = this->mWriteCursor.node;
                        for (size_t i = 0; i < moreNodeCount; ++i) {
                            Node *newNode = new Node();
                            newNode->previous = it->previous;
                            it->previous->next = newNode;
                            it->previous = newNode;
                            newNode->next = it;
                            if(0 == i){
                                uint8_t * src = this->mWriteCursor.node->content;
                                uint8_t * dst = newNode->content;
                                size_t copyLength = this->mWriteCursor.offset - 0;
                                if(copyLength > 0){
                                    memcpy(dst,src,copyLength);
                                }
                                this->mWriteCursor.node = newNode;
                            }
                        }
                        this->mCapacity += moreNodeCount * NodeContentSize;
                    }
                }
            }
        }

        bool CircleBuffer::check(){
            return (this->mWriteCursor.node == this->mReadCursor.node) && (this->mWriteCursor.offset < this->mReadCursor.offset);
        }

        size_t CircleBuffer::retrive(const void * buffer,const size_t bufferCapacity)
        {
            std::lock_guard<std::mutex> lock(this->bufferMutex);
            size_t availableReadLength = std::min(bufferCapacity,this->mUsed);

            //分段拷贝
            size_t leftCopyLength = availableReadLength;
            u_int8_t * dst = (u_int8_t*)buffer;

            while (leftCopyLength > 0 ) {
                void * src = this->mReadCursor.node->content + this->mReadCursor.offset;

                size_t nodeReadableLength = 0;
                if(this->mWriteCursor.node == this->mReadCursor.node && this->mWriteCursor.offset > this->mReadCursor.offset){
                    nodeReadableLength = this->mWriteCursor.offset - this->mReadCursor.offset;
                }else{
                    nodeReadableLength = NodeContentSize - this->mReadCursor.offset;
                }
                size_t  copyLength = std::min(nodeReadableLength,leftCopyLength);
                memcpy((void*)dst, src, copyLength);
                leftCopyLength-=copyLength;
                dst+=copyLength;
                this->mReadCursor.offset += copyLength;
                if (NodeContentSize == this->mReadCursor.offset) {
                    this->mReadCursor.node = this->mReadCursor.node->next;
                    this->mReadCursor.offset = 0;
                }
            }

            this->mUsed-= availableReadLength;
            return availableReadLength;
        }

        size_t CircleBuffer::head(const void * buffer,const size_t bufferLength)
        {
            std::lock_guard<std::mutex> lock(this->bufferMutex);
            size_t availableReadLength = std::min(bufferLength,this->mUsed);

            //创建读指针的副本
            Cursor readCursor_backup = this->mReadCursor;
            
            //分段拷贝
            size_t leftCopyLength = availableReadLength;
            const u_int8_t * dst = (u_int8_t*)buffer;
            while (leftCopyLength > 0 ) {
                void * src = readCursor_backup.node->content + readCursor_backup.offset;
                size_t nodeReadableLength = 0;
                if(this->mWriteCursor.node == readCursor_backup.node && this->mWriteCursor.offset > readCursor_backup.offset){
                    nodeReadableLength = this->mWriteCursor.offset - readCursor_backup.offset;
                }else{
                    nodeReadableLength = NodeContentSize - readCursor_backup.offset;
                }
                size_t  copyLength = std::min(nodeReadableLength,leftCopyLength);
                memcpy((void*)dst, src, copyLength);
                leftCopyLength-=copyLength;
                dst+=copyLength;
                readCursor_backup.offset += copyLength;
                if (NodeContentSize == readCursor_backup.offset) {
                    readCursor_backup.node = readCursor_backup.node->next;
                    readCursor_backup.offset = 0;
                }
            }
            
            return availableReadLength;
        }
        
        size_t CircleBuffer::getUsed() {
            std::lock_guard<std::mutex> lock(this->bufferMutex);
            return this->mUsed;
        }

        CircleBuffer::~CircleBuffer()
        {
            if (this->mCapacity > 0) {
                Node * it = this->mWriteCursor.node->next;
                while (it != this->mWriteCursor.node) {
                    Node * tmp = it;
                    it = it->next;
                    delete tmp;
                }
                delete it;
            }
        }

    }
}
