//
// Created by Kakawater on 2018/1/18.
//

#include <Common/KakaIMMessageAdapter.h>
#include <arpa/inet.h>
namespace kakaIM {
    namespace common {
        struct KakaIMDatagram{
            KakaIMDatagram():dataGramLength(0),messageTypeNameLength(0),messageTypeName(NULL),protobufDataLength(0),protobufData(NULL){

            }
            ~KakaIMDatagram(){
                if (this->messageTypeName){
                    delete[] this->messageTypeName;
                    this->messageTypeName = NULL;
                }
                if (this->protobufData){
                    delete[] this->protobufData;
                    this->protobufData = NULL;
                }
            }
            uint32_t dataGramLength;//数据包长度 = size(dataGramLength) + sizeof(messageTypeNameLength) + messageTypeNameLength + sizeof(protobufDataLength) + protobufDataLength
            uint32_t messageTypeNameLength;//消息类型名称长度
            char * messageTypeName;//消息类型名称
            uint32_t protobufDataLength;//protobuf数据长度
            uint8_t * protobufData;//protobuf数据
        };

        void KakaIMMessageAdapter::encapsulateMessageToByteStream(const ::google::protobuf::Message &message,
                                                    net::CircleBuffer & outputBuffer)const{
            //1.判断消息是否进行了初始化
            if(false == message.IsInitialized()){
                return;
            }
            //2.封装
            struct KakaIMDatagram datagram;

            memset(&datagram,0, sizeof(datagram));
            datagram.messageTypeNameLength = message.GetTypeName().length() + 1;

            datagram.protobufDataLength = message.ByteSize();
	    static uint8_t * buffer = NULL;
            static uint32_t currentBufferLength = 0;
            if (datagram.protobufDataLength > currentBufferLength){
                delete[] buffer;
                buffer = new uint8_t[datagram.protobufDataLength];
                currentBufferLength = datagram.protobufDataLength;
            }
            message.SerializeToArray(buffer, datagram.protobufDataLength);
            datagram.dataGramLength = sizeof(datagram.dataGramLength) + sizeof(datagram.messageTypeNameLength)+datagram.messageTypeNameLength+
                                                                                                               sizeof(datagram.protobufDataLength)+datagram.protobufDataLength;
            //3.将KakaIMDatagram.messageTypeNameLength和将KakaIMDatagram.protobufDataLength转换成网络字节序
            datagram.dataGramLength = htonl(datagram.dataGramLength);
            datagram.messageTypeNameLength = htonl(datagram.messageTypeNameLength);
            datagram.protobufDataLength = htonl(datagram.protobufDataLength);
            //4.将字节流按顺序插入到缓冲区1
            outputBuffer.append(&datagram.dataGramLength, sizeof(datagram.dataGramLength));
            outputBuffer.append(&datagram.messageTypeNameLength, sizeof(datagram.messageTypeNameLength));
            outputBuffer.append(message.GetTypeName().c_str(),ntohl(datagram.messageTypeNameLength));
            outputBuffer.append(&datagram.protobufDataLength, sizeof(datagram.protobufDataLength));
            outputBuffer.append(buffer,ntohl(datagram.protobufDataLength));
        }

        bool KakaIMMessageAdapter::tryToretriveMessage(net::CircleBuffer & inputBuffer, ::google::protobuf::Message **message)const{

            //1.从输入缓冲中提取前面几个字节，判断下一条消息的长度
            static struct KakaIMDatagram datagram;
            static uint32_t currentDatagramMessageTypeNameFieldLength = 0;
            static uint32_t currentDatagramProtobufDataFieldLength = 0;

            if(inputBuffer.getUsed() >= sizeof(datagram.dataGramLength)){
                inputBuffer.head(&datagram.dataGramLength, sizeof(datagram.dataGramLength));
                datagram.dataGramLength = ntohl(datagram.dataGramLength);

                if (inputBuffer.getUsed() >= datagram.dataGramLength){//输入缓冲区中可能存在一条消息
                    //1.提取消息长度并转换成主机字节序
                    inputBuffer.retrive(&datagram.dataGramLength, sizeof(datagram.dataGramLength));
                    datagram.dataGramLength = ntohl(datagram.dataGramLength);
                    //2.提取消息类型名称长度并转换成主机字节序
                    inputBuffer.retrive(&datagram.messageTypeNameLength, sizeof(datagram.messageTypeNameLength));
                    datagram.messageTypeNameLength = ntohl(datagram.messageTypeNameLength);
                    //3.提取消息类型名称
                    if (datagram.messageTypeNameLength > currentDatagramMessageTypeNameFieldLength){
                        if (datagram.messageTypeName){
                            delete[] datagram.messageTypeName;
                            datagram.messageTypeName = NULL;
                        }
                        datagram.messageTypeName = new char[datagram.messageTypeNameLength];
                        currentDatagramMessageTypeNameFieldLength = datagram.messageTypeNameLength;
                    }
                    datagram.messageTypeName[datagram.messageTypeNameLength] = '\0';
                    inputBuffer.retrive(datagram.messageTypeName,datagram.messageTypeNameLength);
                    //4.提取protobufData长度并转换成主机字节序
                    inputBuffer.retrive(&datagram.protobufDataLength, sizeof(datagram.protobufDataLength));
                    datagram.protobufDataLength = ntohl(datagram.protobufDataLength);
                    //5.提取protobufData
                    if (datagram.protobufDataLength > currentDatagramProtobufDataFieldLength){
                        if (datagram.protobufData){
                            delete[] datagram.protobufData;
                            datagram.protobufData = NULL;
                        }
                        datagram.protobufData = new uint8_t[datagram.protobufDataLength];
                        currentDatagramProtobufDataFieldLength = datagram.protobufDataLength;
                    }
                    inputBuffer.retrive(datagram.protobufData,datagram.protobufDataLength);
                    //6.根据messageTypeName创建消息，并进行反序列化
                    auto descriptor = ::google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(datagram.messageTypeName);
                    if(nullptr == descriptor){//不存在此消息类型
                        return false;
                    }
                    ::google::protobuf::Message * tmpMessage = ::google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor)->New();
                    tmpMessage->ParseFromArray(datagram.protobufData,datagram.protobufDataLength);

                    //8.判断消息是否初始化成功
                    if(tmpMessage->IsInitialized()){
                        *message = tmpMessage;
                        return true;
                    }else{
                        delete tmpMessage;
                        return false;
                    }
                }
            }
            return false;
        }
    }
}

