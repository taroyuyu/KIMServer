cmake_minimum_required(VERSION 3.2)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
# 项目名
project(KakaIMCluster)
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES mainPresident.cpp President/President.cpp Common/KIMModule.h President/ClusterManagerModule/ClusterManagerModule.cpp President/UserStateManagerModule/UserStateManagerModule.cpp Common/Net/MessageCenterModule/MessageCenterModule.cpp Common/KakaIMMessageAdapter.cpp Common/KIMModule.cpp Common/Net/TCPSocket/TCPServerSocket.cpp Common/Net/TCPSocket/TCPClientSocket.cpp Common/Net/TCPSocket/TCPSocket.cpp Common/Net/Buffer/CircleBuffer.cpp Common/proto/MessageCluster.pb.cc Common/proto/KakaIMClientPresident.pb.cc President/ServerRelayModule/ServerRelayModule.cpp Common/util/Date.cpp President/MessageIDGenerateModule/MessageIDGenerateModule.cpp President/NodeLoadBlanceModule/NodeLoadBlanceModule.cpp)
add_executable(KakaIMClusterPresident ${SOURCE_FILES})
target_link_libraries(KakaIMClusterPresident yaml-cpp log4cxx)
target_link_libraries(KakaIMClusterPresident ${Protobuf_LIBRARIES})

set(KakaIMClusterNode_SOURCE_FILES mainNode.cpp Node/Node.cpp Common/KIMModule.cpp Node/ClusterModule/ClusterModule.cpp Common/proto/KakaIMMessage.pb.cc Common/proto/MessageCluster.pb.cc  Common/Net/TCPSocket/TCPSocket.cpp Common/Net/TCPSocket/TCPClientSocket.cpp Common/Net/TCPSocket/TCPServerSocket.cpp Common/Net/TCPSocketManager/TCPSocketManager.cpp Common/Net/Buffer/CircleBuffer.cpp Common/util/Date.cpp Common/util/MD5.cpp Common/Net/MessageCenterModule/MessageCenterModule.cpp Node/SessionModule/SessionModule.cpp Common/EventBus/EventBus.cpp Common/EventBus/Event.cpp Common/KakaIMMessageAdapter.cpp Node/AuthenticationModule/AuthenticationModule.cpp Node/OnlineStateModule/OnlineStateModule.cpp Node/MessageSendServiceModule/MessageSendServiceModule.cpp Node/RosterModule/RosterModule.cpp Node/SingleChatModule/SingleChatModule.cpp Node/OfflineModule/OfflineModule.cpp Node/GroupCharModule/GroupChatModule.cpp)
add_executable(KakaIMClusterNode ${KakaIMClusterNode_SOURCE_FILES})
target_link_libraries(KakaIMClusterNode -lpqxx -lpq yaml-cpp uuid log4cxx)
target_link_libraries(KakaIMClusterNode ${Protobuf_LIBRARIES})
