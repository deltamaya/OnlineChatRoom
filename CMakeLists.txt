cmake_minimum_required(VERSION 3.28)
project(online_chatroom)
set(CMAKE_CXX_STANDARD 20)
find_package(protobuf REQUIRED)
find_package(absl REQUIRED)
link_directories(${PROTOBUF_LIBRARY_DIRS})
add_definitions(${PROTOBUF_LIBRARY_DIRS})
include_directories(
        ${PROTOBUF_LIBRARY_DIRS}
)
include_directories(./include)
aux_source_directory(./src/server SERVERSRC)
add_executable(server ${SERVERSRC}
        src/message.pb.cc
        src/message.grpc.pb.cc
)
target_link_libraries(server PUBLIC jsoncpp pthread mysqlpp   protobuf::libprotobuf)


aux_source_directory(./src/client CLIENTSRC)
add_executable(client ${CLIENTSRC}
        src/message.pb.cc
        src/message.grpc.pb.cc)
target_link_libraries(client PUBLIC jsoncpp pthread mysqlpp  protobuf::libprotobuf)

