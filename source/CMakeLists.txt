cmake_minimum_required(VERSION 2.8)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
# 项目名
project(KakaIMCluster)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -I /usr/local/include/ -I /usr/lib64/ -L /usr/local/lib/ -lprotobuf -lz -lrt -lm -lpthread -g")

set(SOURCE_FILES mainPresident.cpp President/President.cpp Common/KIMModule.h President/ClusterManagerModule/ClusterManagerModule.cpp President/UserStateManagerModule/UserStateManagerModule.cpp Common/Net/MessageCenterModule/MessageCenterModule.cpp Common/KakaIMMessageAdapter.cpp Common/KIMModule.cpp Common/Net/TCPSocket/TCPServerSocket.cpp Common/Net/TCPSocket/TCPClientSocket.cpp Common/Net/TCPSocket/TCPSocket.cpp Common/Net/Buffer/CircleBuffer.cpp Common/proto/MessageCluster.pb.cc Common/proto/KakaIMClientPresident.pb.cc President/ServerRelayModule/ServerRelayModule.cpp Common/util/Date.cpp President/MessageIDGenerateModule/MessageIDGenerateModule.cpp President/NodeLoadBlanceModule/NodeLoadBlanceModule.cpp)
add_executable(KakaIMClusterPresident ${SOURCE_FILES})
target_link_libraries(KakaIMClusterPresident yaml-cpp log4cxx)

set(KakaIMClusterNode_SOURCE_FILES mainNode.cpp Node/Node.cpp Common/KIMModule.cpp Node/ClusterModule/ClusterModule.cpp Common/proto/KakaIMMessage.pb.cc Common/proto/MessageCluster.pb.cc  Common/Net/TCPSocket/TCPSocket.cpp Common/Net/TCPSocket/TCPClientSocket.cpp Common/Net/TCPSocket/TCPServerSocket.cpp Common/Net/TCPSocketManager/TCPSocketManager.cpp Common/Net/Buffer/CircleBuffer.cpp Common/util/Date.cpp Common/util/MD5.cpp Common/Net/MessageCenterModule/MessageCenterModule.cpp Node/SessionModule/SessionModule.cpp Common/EventBus/EventBus.cpp Common/EventBus/Event.cpp Common/KakaIMMessageAdapter.cpp Node/AuthenticationModule/AuthenticationModule.cpp Node/OnlineStateModule/OnlineStateModule.cpp Node/MessageSendServiceModule/MessageSendServiceModule.cpp Node/RosterModule/RosterModule.cpp Node/SingleChatModule/SingleChatModule.cpp Node/OfflineModule/OfflineModule.cpp Node/GroupCharModule/GroupChatModule.cpp)
add_executable(KakaIMClusterNode ${KakaIMClusterNode_SOURCE_FILES})
target_link_libraries(KakaIMClusterNode -lpqxx -lpq yaml-cpp uuid log4cxx)

#cmake_policy(SET CMP0037 OLD)
cmake_policy(SET CMP0015 OLD)
