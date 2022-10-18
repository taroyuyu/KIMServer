//
// Created by Kakawater on 2018/3/16.
//


#include <zconf.h>
#include <math.h>
#include <typeinfo>
#include <President/NodeLoadBlanceModule/NodeLoadBlanceModule.h>
#include <President/Log/log.h>

namespace kakaIM {
    namespace president {
        NodeLoadBlanceModule::NodeLoadBlanceModule() : KIMPresidentModule(NodeLoadBlanceModuleLogger){
        }

        NodeLoadBlanceModule::~NodeLoadBlanceModule() {
        }

        void NodeLoadBlanceModule::addEvent(ClusterEvent event) {
            this->mEventQueue.push(event);
        }

        void NodeLoadBlanceModule::execute() {
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Started;
                this->m_statusCV.notify_all();
            }
            while (not this->m_needStop) {
                bool needSleep = true;
                if (auto task = this->mTaskQueue.try_pop()) {
                    this->dispatchMessage(*task);
                    needSleep = false;
                }

                if (auto event = this->mEventQueue.try_pop()){
                    this->dispatchClusterEvent(*event);
                    needSleep = false;
                }

                if (needSleep){
                    std::this_thread::yield();
                }
            }

            this->m_needStop = false;
            {
                std::lock_guard<std::mutex> lock(this->m_statusMutex);
                this->m_status = Status::Stopped;
                this->m_statusCV.notify_all();
            }
        }

        void NodeLoadBlanceModule::dispatchMessage(std::pair<std::unique_ptr<::google::protobuf::Message>, const std::string> & task){
            std::string messageType = task.first->GetTypeName();
            if (messageType == NodeLoadInfoMessage::default_instance().GetTypeName()) {
                this->handleNodeLoadInfoMessage(
                        *(const NodeLoadInfoMessage *) task.first.get(), task.second);
            } else if (messageType == RequestNodeMessage::default_instance().GetTypeName()) {
                this->handleRequestNodeMessage(*(const RequestNodeMessage *) task.first.get(),
                                               task.second);
            }
        }

        void NodeLoadBlanceModule::dispatchClusterEvent(ClusterEvent & event){
            switch (event.getEventType()) {
                case ClusterEvent::ClusterEventType::NewNodeJoinedCluster: {
                    this->handleNewNodeJoinedClusterEvent(event);
                }
                    break;
                case ClusterEvent::ClusterEventType::NodeRemovedCluster: {
                    this->handleNodeRemovedClusterEvent(event);
                }
                    break;
                default:
                    break;
            }
        }

        void NodeLoadBlanceModule::handleNodeLoadInfoMessage(const NodeLoadInfoMessage &message,
                                                             const std::string connectionIdentifier) {
            auto serverManageService = this->mServerManageServicePtr.lock();
            if (!serverManageService) {
                return;;
            }
            try {
                Node node = serverManageService->getNodeWithConnectionIdentifier(connectionIdentifier);
                NodeLoadInfo loadInfo(message.connectioncount(), message.cpuusage(), message.memusage());
                std::lock_guard<std::mutex> lock(this->nodeLoadInfoSetMutex);
                auto nodeLoadInfoIt = this->nodeLoadInfoSet.find(node);
                if (nodeLoadInfoIt == this->nodeLoadInfoSet.end()) {
                    this->nodeLoadInfoSet.emplace(node, loadInfo);
                } else {
                    this->nodeLoadInfoSet[node] = loadInfo;
                }
            } catch (ServerManageService::NodeNotExitException &exception) {
                LOG4CXX_ERROR(this->logger,
                              typeid(this).name() << "" << __FUNCTION__ << "找不到connectionIdentifier对应的Node");
            }
        }

        int NodeLoadBlanceModule::calculateDistance(std::pair<float, float> point_1, std::pair<float, float> point_2) {
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
            double distance = 2 * asin(sqrt(pow(sin(lat_distance / 2), 2) +
                                            cos(point_1_lat_rad) * cos(point_2_lat_rad) *
                                            pow(sin(lng_distance / 2), 2)));
            distance = distance * EARTH_RADIUS;
            distance = (int) (distance * 10000000) / 10000;
            return distance;
        }

        void NodeLoadBlanceModule::handleRequestNodeMessage(const RequestNodeMessage &message,
                                                            const std::string connectionIdentifier) {
            auto connectionOperationService = this->connectionOperationServicePtr.lock();
            if (!connectionOperationService) {
                return;
            }
            const std::string userAccount = message.useraccount();
            std::pair<float, float> clientLngLatPair;
            clientLngLatPair.first = message.longitude();
            clientLngLatPair.second = message.latitude();

            ResponseNodeMessage responseNodeMessage;
            auto userStateManagerService = this->mUserStateManagerServicePtr.lock();
            if (!userStateManagerService) {
                responseNodeMessage.set_errortype(ResponseNodeMessage_Error_ServerInternalError);
                connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseNodeMessage);
                return;
            }

            auto loginList = userStateManagerService->queryUserLoginServer(userAccount);

            if (loginList.size()) {//用户已经登陆了某台节点,则直接返回该节点
                auto serverManageService = this->mServerManageServicePtr.lock();
                if (!serverManageService) {
                    responseNodeMessage.set_errortype(ResponseNodeMessage_Error_ServerInternalError);
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseNodeMessage);
                    return;
                }

                bool getNodeInfoSuccess = false;
                try {
                    Node loginNode = serverManageService->getNode(loginList.front());
                    auto node = responseNodeMessage.add_node();
                    node->set_ip_addr(loginNode.getServiceIpAddress());
                    node->set_port(loginNode.getServicePort());
                    getNodeInfoSuccess = true;
                } catch (ServerManageService::NodeNotExitException &exception) {
                    getNodeInfoSuccess = false;
                }

                if (!getNodeInfoSuccess) {
                    goto unlogin;
                }

            } else {//用户当前并未登陆任何设备
                unlogin:
                //1.获取集群中所有的节点
                auto serverManageService = this->mServerManageServicePtr.lock();
                if (!serverManageService) {
                    responseNodeMessage.set_errortype(ResponseNodeMessage_Error_ServerInternalError);
                    connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseNodeMessage);
                    return;
                }
                auto nodeSet = serverManageService->getAllNodes();
                //2、根据服务器的负载情况,筛选出正常的服务器
                std::vector<decltype(nodeSet.begin())> needEraseIterators;
                for (auto nodeIt = nodeSet.begin(); nodeIt != nodeSet.end(); ++nodeIt) {
                    std::cout << "nodeIt->getServiceIpAddress()=" << nodeIt->getServiceIpAddress() << std::endl;
                    auto nodeLoadInfoIt = this->nodeLoadInfoSet.find(*nodeIt);
                    if (nodeLoadInfoIt == this->nodeLoadInfoSet.end()) {
                        needEraseIterators.push_back(nodeIt);
                    } else {
                        if (nodeLoadInfoIt->second.cpuUsage > 0.9 || nodeLoadInfoIt->second.memUsage > 0.9) {
                            needEraseIterators.push_back(nodeIt);
                        }
                    }
                }
                for(auto nodeIt : needEraseIterators){
                    nodeSet.erase(nodeIt);
                }
                //4、根据服务器与客户端的距离，挑选出离客户端最近的节点
                std::list<std::pair<Node, int >> nodeList;
                //4.1计算每个可用节点与客户端之间的距离
                for (auto node: nodeSet) {
                    int distance = calculateDistance(clientLngLatPair, node.getServerLngLatPair());;
                    nodeList.emplace_back(node, distance);
                }
                //4.2排序
                nodeList.sort([](const std::list<std::pair<Node, int >>::value_type left,
                                 const std::list<std::pair<Node, int >>::value_type right) -> bool {
                    return left.second < right.second;
                });
                /**
                 * 每次最多提供五台Node的信息
                 */
                int i = 0;
                for (auto it = nodeList.begin(); i < 5 && it != nodeList.end(); ++it, ++i) {
                    auto node = responseNodeMessage.add_node();
                    node->set_ip_addr(it->first.getServiceIpAddress());
                    node->set_port(it->first.getServicePort());
                }
            }

            connectionOperationService->sendMessageThroughConnection(connectionIdentifier, responseNodeMessage);
        }

        void NodeLoadBlanceModule::handleNewNodeJoinedClusterEvent(const ClusterEvent &event) {

        }

        void NodeLoadBlanceModule::handleNodeRemovedClusterEvent(const ClusterEvent &event) {

        }
    }
}
