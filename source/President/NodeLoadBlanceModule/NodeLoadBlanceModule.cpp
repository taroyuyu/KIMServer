//
// Created by taroyuyu on 2018/3/16.
//


#include <zconf.h>
#include <math.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <typeinfo>
#include <typeinfo>
#include "NodeLoadBlanceModule.h"
#include "../Log/log.h"

namespace kakaIM {
    namespace president {
        NodeLoadBlanceModule::NodeLoadBlanceModule():mEpollInstance(-1),messageEventfd(-1),clusterEventfd(-1){
	    this->logger = log4cxx::Logger::getLogger(NodeLoadBlanceModuleLogger);
        }
        NodeLoadBlanceModule::~NodeLoadBlanceModule(){
        }

        bool NodeLoadBlanceModule::init(){
            this->messageEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->messageEventfd < 0) {
                return false;
            }
            this->clusterEventfd = ::eventfd(0, EFD_SEMAPHORE);
            if (this->clusterEventfd < 0) {
                return false;
            }
            //创建Epoll实例
            if (-1 == (this->mEpollInstance = epoll_create1(0))) {
                return false;
            }
            //向Epill实例注册messageEventfd,和clusterEventfd
            struct epoll_event messageEventfdEvent;
            messageEventfdEvent.events = EPOLLIN;
            messageEventfdEvent.data.fd = this->messageEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->messageEventfd, &messageEventfdEvent)) {
                return false;
            }
            struct epoll_event clusterEventfdEvent;
            clusterEventfdEvent.events = EPOLLIN;
            clusterEventfdEvent.data.fd = this->clusterEventfd;
            if (-1 == epoll_ctl(this->mEpollInstance, EPOLL_CTL_ADD, this->clusterEventfd, &clusterEventfdEvent)) {
                return false;
            }
            return true;
        }

        void NodeLoadBlanceModule::setConnectionOperationService(std::weak_ptr<ConnectionOperationService> connectionOperationServicePtr){
            this->mConnectionOperationServicePtr = connectionOperationServicePtr;
        }

        void NodeLoadBlanceModule::setServerManageService(std::weak_ptr<ServerManageService> serverManageServicePtr){
            this->mServerManageServicePtr = serverManageServicePtr;
        }

	void NodeLoadBlanceModule::setUserStateManagerService(std::weak_ptr<UserStateManagerService> userStateManagerServicePtr){
            this->mUserStateManagerServicePtr = userStateManagerServicePtr;
        }

	void NodeLoadBlanceModule::addNodeLoadInfoMessage(const NodeLoadInfoMessage & message,const std::string connectionIdentifier){
            std::unique_ptr<NodeLoadInfoMessage> nodeLoadInfoMessage(new NodeLoadInfoMessage(message));
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(nodeLoadInfoMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void NodeLoadBlanceModule::addRequestNodeMessage(const RequestNodeMessage & message,const std::string connectionIdentifier){
            std::unique_ptr<RequestNodeMessage> requestNodeMessage(new RequestNodeMessage(message));
            std::lock_guard<std::mutex> lock(this->messageQueueMutex);
            this->messageQueue.emplace(std::move(requestNodeMessage), connectionIdentifier);
            uint64_t count = 1;
            //增加信号量
            ::write(this->messageEventfd, &count, sizeof(count));
        }

        void NodeLoadBlanceModule::addEvent(ClusterEvent event){
            std::lock_guard<std::mutex> lock(this->eventQueueMutex);
            this->mEventQueue.emplace(event);
            uint64_t count = 1;
            //增加信号量
            ::write(this->clusterEventfd, &count, sizeof(count));
        }

        void NodeLoadBlanceModule::execute(){
            while (this->m_isStarted) {
                int const kHandleEventMaxCountPerLoop = 2;
                static struct epoll_event happedEvents[kHandleEventMaxCountPerLoop];

                //等待事件发送，超时时间为1秒
                int happedEventsCount = epoll_wait(this->mEpollInstance, happedEvents, kHandleEventMaxCountPerLoop,
                                                   10000);

                if (-1 == happedEventsCount) {
		    LOG4CXX_WARN(this->logger, typeid(this).name()<<""<<__FUNCTION__<<"等待mEpollInstance出错="<<errno);
                }

                //遍历所有的文件描述符
                for (int i = 0; i < happedEventsCount; ++i) {
                    if (EPOLLIN & happedEvents[i].events) {
                        if (this->clusterEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < read(this->clusterEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->mEventQueue.empty()) {
                                    this->eventQueueMutex.lock();
                                    auto event = std::move(this->mEventQueue.front());
                                    this->mEventQueue.pop();
                                    this->eventQueueMutex.unlock();

                                    switch (event.getEventType()) {
                                        case ClusterEvent::NewNodeJoinedCluster: {
                                            this->handleNewNodeJoinedClusterEvent(event);
                                        }
                                            break;
                                        case ClusterEvent::NodeRemovedCluster: {
                                            this->handleNodeRemovedClusterEvent(event);
                                        }
                                            break;
                                        default:
                                            break;
                                    }
                                }
                            }
                        } else if (this->messageEventfd == happedEvents[i].data.fd) {
                            uint64_t count;
                            if (0 < read(this->messageEventfd, &count, sizeof(count))) {
                                while (count-- && false == this->messageQueue.empty()) {
                                    this->messageQueueMutex.lock();
                                    auto pairIt = std::move(this->messageQueue.front());
                                    this->messageQueue.pop();
                                    this->messageQueueMutex.unlock();

                                    std::string messageType = pairIt.first->GetTypeName();
				    if(messageType == NodeLoadInfoMessage::default_instance().GetTypeName()){
                                        this->handleNodeLoadInfoMessage(*(const NodeLoadInfoMessage *) pairIt.first.get(), pairIt.second);
                                    }else if (messageType == RequestNodeMessage::default_instance().GetTypeName()) {
                                        this->handleRequestNodeMessage(*(const RequestNodeMessage *) pairIt.first.get(), pairIt.second);
                                    }else{

                                    }
                                }
                            } else {
				LOG4CXX_WARN(this->logger, typeid(this).name()<<""<<__FUNCTION__<<"read(messageEventfd)操作出错，errno ="<<errno);
                            }
                        }
                    } else {

                    }
                }
            }
        }

	void NodeLoadBlanceModule::handleNodeLoadInfoMessage(const NodeLoadInfoMessage & message,const std::string connectionIdentifier){
            auto serverManageService = this->mServerManageServicePtr.lock();
            if(!serverManageService){
                return;;
            }
            try {
                Node node = serverManageService->getNodeWithConnectionIdentifier(connectionIdentifier);
                NodeLoadInfo loadInfo(message.connectioncount(),message.cpuusage(),message.memusage());
                std::lock_guard<std::mutex> lock(this->nodeLoadInfoSetMutex);
                auto nodeLoadInfoIt = this->nodeLoadInfoSet.find(node);
                if(nodeLoadInfoIt == this->nodeLoadInfoSet.end()){
                    this->nodeLoadInfoSet.emplace(node,loadInfo);
                }else{
                    this->nodeLoadInfoSet[node] = loadInfo;
                }
            }catch (ServerManageService::NodeNotExitException & exception){
		LOG4CXX_ERROR(this->logger, typeid(this).name()<<""<<__FUNCTION__<<"找不到connectionIdentifier对应的Node");
            }
        }

	int NodeLoadBlanceModule::calculateDistance(std::pair<float,float> point_1,std::pair<float,float> point_2)
        {
            const float EARTH_RADIUS = 6378.137;
            const double PI = 3.1415926535898;

        //将经度转换为其对应的弧度
            double point_1_Lng_rad = PI * (point_1.first / 180.0);
            double point_2_Lng_rad = PI * (point_2.first / 180.0);
            double lng_distance = point_1_Lng_rad - point_2_Lng_rad;
        //将纬度转换为其对应的弧度
            double point_1_lat_rad = PI * (point_1.second / 180.0);
            double point_2_lat_rad = PI * (point_2.second / 180.0);
            double lat_distance = point_1_lat_rad - point_2_lat_rad;
            double distance = 2 * asin(sqrt(pow(sin(lat_distance/2),2) + cos(point_1_lat_rad)*cos(point_2_lat_rad)*pow(sin(lng_distance/2),2)));
            distance = distance * EARTH_RADIUS;
            distance = (int)(distance * 10000000) / 10000;
            return distance;
        }

        void NodeLoadBlanceModule::handleRequestNodeMessage(const RequestNodeMessage & message,const std::string connectionIdentifier){
	    auto connectionOperationService =  this->mConnectionOperationServicePtr.lock();
            if(!connectionOperationService){
                return;
            }
            const std::string userAccount = message.useraccount();
            std::pair<float,float> clientLngLatPair;
            clientLngLatPair.first = message.longitude();
            clientLngLatPair.second = message.latitude();

            ResponseNodeMessage responseNodeMessage;
            auto userStateManagerService =  this->mUserStateManagerServicePtr.lock();
            if(!userStateManagerService){
                responseNodeMessage.set_errortype(ResponseNodeMessage_Error_ServerInternalError);
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier,responseNodeMessage);
                return;
            }

            auto loginList = userStateManagerService->queryUserLoginServer(userAccount);

            if(loginList.size()){//用户已经登陆了某台节点,则直接返回该节点
                auto serverManageService = this->mServerManageServicePtr.lock();
                if(!serverManageService){
                    responseNodeMessage.set_errortype(ResponseNodeMessage_Error_ServerInternalError);
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier,responseNodeMessage);
                    return;
                }
                Node loginNode = serverManageService->getNode(loginList.front());

                auto node = responseNodeMessage.add_node();
                node->set_ip_addr(loginNode.getServerIpAddress());
                node->set_port(loginNode.getServerPort());

            }else{//用户当前并未登陆任何设备
                //1.获取客户端的IP地址
                auto pair = connectionOperationService->queryConnectionAddress(connectionIdentifier);

                if(!pair.first){
                    responseNodeMessage.set_errortype(ResponseNodeMessage_Error_ServerInternalError);
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier,responseNodeMessage);
                    return;
                }
                const std::string clientIpAddress = pair.second.first;
                //2.获取集群中所有的节点
                auto serverManageService = this->mServerManageServicePtr.lock();
                if(!serverManageService){
                    responseNodeMessage.set_errortype(ResponseNodeMessage_Error_ServerInternalError);
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier,responseNodeMessage);
                    return;
                }
                auto nodeSet = serverManageService->getAllNodes();
                //3、根据服务器的负载情况,筛选出正常的服务器
		for(auto nodeIt = nodeSet.begin();nodeIt != nodeSet.end();++nodeIt){
                    auto nodeLoadInfoIt = this->nodeLoadInfoSet.find(*nodeIt);
                    if(nodeLoadInfoIt == this->nodeLoadInfoSet.end()){
                        nodeSet.erase(nodeIt);
                    }else{
                        if (nodeLoadInfoIt->second.cpuUsage > 0.9 || nodeLoadInfoIt->second.memUsage > 0.9){
                            nodeSet.erase(nodeIt);
                        }                    }
                }

                //4、根据服务器与客户端的距离，挑选出离客户端最近的节点
                std::list<std::pair<Node,int >> nodeList;
                    //4.1计算每个可用节点与客户端之间的距离
                for (auto node : nodeSet) {
                    int distance = calculateDistance(clientLngLatPair,node.getServerLngLatPair());;
                    nodeList.emplace_back(node,distance);
                }
                    //4.2排序
                nodeList.sort([](const std::list<std::pair<Node,int >>::value_type left, const std::list<std::pair<Node,int >>::value_type right)->bool {
                    return left.second < right.second;
                });
                    /**
                     * 每次最多提供五台Node的信息
                     */
                int i = 0;
                for (auto it = nodeList.begin();i < 5 && it != nodeList.end();++it,++i) {
                    auto node = responseNodeMessage.add_node();
                    node->set_ip_addr(it->first.getServerIpAddress());
                    node->set_port(it->first.getServerPort());
                }
            }

            connectionOperationService->sendMessageThroughConnection(connectionIdentifier,responseNodeMessage);
        }

        void NodeLoadBlanceModule::handleNewNodeJoinedClusterEvent(const ClusterEvent &event){

        }

        void NodeLoadBlanceModule::handleNodeRemovedClusterEvent(const ClusterEvent &event){

        }
    }
}
