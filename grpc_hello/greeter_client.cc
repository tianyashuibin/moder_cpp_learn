#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpcpp/create_channel.h> 
#include <grpcpp/security/credentials.h> 
#include "helloworld.grpc.pb.h" // 编译 proto 文件后生成

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloRequest;
using helloworld::HelloReply;

// 1. 客户端类
class GreeterClient {
public:
    // 构造函数：创建 gRPC Channel 连接到服务器
    GreeterClient(std::shared_ptr<Channel> channel)
        : stub_(Greeter::NewStub(channel)) {}

    // 调用 SayHello RPC 方法
    std::string SayHello(const std::string& user) {
        // 构建请求消息
        HelloRequest request;
        request.set_name(user);

        // 响应消息和客户端上下文
        HelloReply reply;
        ClientContext context;

        // 实际的 RPC 调用
        Status status = stub_->SayHello(&context, request, &reply);

        // 检查调用状态
        if (status.ok()) {
            return reply.message();
        } else {
            // 打印错误信息
            std::cout << "RPC failed. Error code: " << status.error_code()
                      << ", message: " << status.error_message() << std::endl;
            return "RPC FAILED";
        }
    }

private:
    // 存根（Stub）：客户端用于调用远程方法的本地代理
    std::unique_ptr<Greeter::Stub> stub_;
};

int main() {
    std::string target_str = "localhost:50051";
    
    // 创建 Channel，连接到服务器
    GreeterClient greeter(
        grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    
    std::string user = "World";
    
    // 调用 RPC 方法
    std::string reply = greeter.SayHello(user);
    
    std::cout << "Client received: " << reply << std::endl;

    return 0;
}