cmake_minimum_required(VERSION 3.10)
project(GrpcExample)

# 查找 gRPC 和 Protobuf (用于链接库)
find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)

# Protobuf 文件和 gRPC 生成的文件列表
# 我们现在手动指定这些文件，因为它们已经被生成了
set(GENERATED_SRCS
    helloworld.pb.cc
    helloworld.grpc.pb.cc
)

# 客户端和服务器的源文件
set(CLIENT_SRCS greeter_client.cc ${GENERATED_SRCS})
set(SERVER_SRCS greeter_server.cc ${GENERATED_SRCS})

# 1. 编译客户端可执行文件
add_executable(greeter_client ${CLIENT_SRCS})
target_link_libraries(greeter_client
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)
# 确保编译器能找到头文件（包括生成的 .h 文件）
target_include_directories(greeter_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})


# 2. 编译服务器可执行文件
add_executable(greeter_server ${SERVER_SRCS})
target_link_libraries(greeter_server
    gRPC::grpc++
    gRPC::grpc
    protobuf::libprotobuf
)
target_include_directories(greeter_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})