#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "helloworld.grpc.pb.h" // 编译 proto 文件后生成

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloRequest;
using helloworld::HelloReply;

// 1. 服务实现类
class GreeterServiceImpl final : public Greeter::Service {
    // 实现 SayHello RPC 方法
    Status SayHello(ServerContext* context, const HelloRequest* request,
                    HelloReply* reply) override {
        // 从请求中获取 name
        std::string prefix("Hello ");
        // 构造响应消息
        reply->set_message(prefix + request->name());
        std::cout << "Received: " << request->name() << std::endl;
        // 返回 OK 状态
        return Status::OK;
    }
};

// 2. 启动服务器的函数
void RunServer() {
    std::string server_address("0.0.0.0:50051");
    GreeterServiceImpl service;

    ServerBuilder builder;
    // 监听指定端口，并禁用认证（InsecureChannelCredentials）
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // 注册服务
    builder.RegisterService(&service);

    // 启动服务器
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 等待服务器关闭（保持运行状态）
    server->Wait();
}

int main() {
    RunServer();
    return 0;
}