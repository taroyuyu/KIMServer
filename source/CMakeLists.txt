cmake_minimum_required(VERSION 3.2)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

project(KakaIMCluster)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR})

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SOURCE_FILES mainPresident.cpp President/President.cpp Common/KIMModule.h President/ClusterManagerModule/ClusterManagerModule.cpp President/UserStateManagerModule/UserStateManagerModule.cpp Common/Net/MessageCenterModule/MessageCenterModule.cpp Common/KakaIMMessageAdapter.cpp Common/KIMModule.cpp Common/Net/TCPSocket/TCPServerSocket.cpp Common/Net/TCPSocket/TCPClientSocket.cpp Common/Net/TCPSocket/TCPSocket.cpp Common/Net/Buffer/CircleBuffer.cpp Common/proto/MessageCluster.pb.cc Common/proto/KakaIMClientPresident.pb.cc President/ServerRelayModule/ServerRelayModule.cpp Common/util/Date.cpp President/MessageIDGenerateModule/MessageIDGenerateModule.cpp President/NodeLoadBlanceModule/NodeLoadBlanceModule.cpp)
add_executable(KakaIMClusterPresident ${SOURCE_FILES})
target_link_libraries(KakaIMClusterPresident yaml-cpp log4cxx)
target_link_libraries(KakaIMClusterPresident ${Protobuf_LIBRARIES})

set(KakaIMClusterNode_SOURCE_FILES mainNode.cpp Node/Node.cpp Common/KIMModule.cpp Node/KIMNodeModule/KIMNodeModule.cpp Node/ClusterModule/ClusterModule.cpp Common/proto/KakaIMMessage.pb.cc Common/proto/MessageCluster.pb.cc Common/proto/KakaIMRPC.pb.cc  Common/Net/TCPSocket/TCPSocket.cpp Common/Net/TCPSocket/TCPClientSocket.cpp Common/Net/TCPSocket/TCPServerSocket.cpp Common/Net/TCPSocketManager/TCPSocketManager.cpp Common/Net/Buffer/CircleBuffer.cpp Common/util/Date.cpp Common/util/MD5.cpp Common/Net/MessageCenterModule/MessageCenterModule.cpp Node/SessionModule/SessionModule.cpp Common/EventBus/EventBus.cpp Common/EventBus/Event.cpp Common/KakaIMMessageAdapter.cpp Node/AuthenticationModule/AuthenticationModule.cpp Node/OnlineStateModule/OnlineStateModule.cpp Node/MessageSendServiceModule/MessageSendServiceModule.cpp Node/RosterModule/RosterModule.cpp Node/SingleChatModule/SingleChatModule.cpp Node/OfflineModule/OfflineModule.cpp Node/GroupChatModule/GroupChatModule.cpp)
add_executable(KakaIMClusterNode ${KakaIMClusterNode_SOURCE_FILES})
target_link_libraries(KakaIMClusterNode pq pqxx yaml-cpp uuid log4cxx SimpleAmqpClient)
target_link_libraries(KakaIMClusterNode ${Protobuf_LIBRARIES})

set(KakaIMClient_SOURCE_FILES Client/mainClient.cpp Common/proto/KakaIMMessage.pb.cc Common/KIMModule.cpp Common/Net/TCPSocket/TCPClientSocket.cpp Common/Net/Buffer/CircleBuffer.cpp Common/Net/TCPSocketManager/TCPSocketManager.cpp Common/KakaIMMessageAdapter.cpp Common/util/Date.cpp Client/SessionModule/SessionModule.cpp Client/AuthenticationModule/AuthenticationModule.cpp  Client/OnlineStateModule/OnlineStateModule.cpp Client/OnlineStateModule/OnlineStateModule.h  Client/RosterModule/RosterModule.cpp Client/SingleChatModule/SingleChatModule.cpp Client/GroupChatModule/GroupChatModule.cpp Client/KakaIMClient.cpp)
add_executable(KakaIMClient ${KakaIMClient_SOURCE_FILES})
target_link_libraries(KakaIMClient ${Protobuf_LIBRARIES})