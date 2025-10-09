#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

#include <grpcpp/grpcpp.h>
#include "helloworld.grpc.pb.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::CompletionQueue;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloRequest;
using helloworld::HelloReply;

// ==========================================================
// 1. 简易线程池实现 (与之前相同)
// ==========================================================
class ThreadPool {
public:
    ThreadPool(size_t threads) : stop_flag(false) {
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back(
                [this] {
                    for(;;) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex);
                            condition.wait(lock, [this]{ return stop_flag || !tasks.empty(); });
                            if(stop_flag && tasks.empty()) return;
                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                }
            );
        }
    }

    void enqueue(std::function<void()> f) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if(stop_flag) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace(std::move(f));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop_flag = true;
        }
        condition.notify_all();
        for(std::thread &worker : workers)
            if(worker.joinable())
                worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop_flag;
};

// ==========================================================
// 2. 异步服务器实现类
// ==========================================================
class AsyncGreeterServer final {
public:
    // 构造函数中启动业务线程池
    AsyncGreeterServer(size_t num_business_threads) 
        : business_thread_pool_(num_business_threads) {}

    ~AsyncGreeterServer() {
        server_->Shutdown();
        // 必须在 Shutdown 后排空所有待处理事件，否则程序可能崩溃
        for (auto& cq : cqs_) {
            void* tag;
            bool ok;
            while (cq->Next(&tag, &ok)) {}
        }
    }

    void Run() {
        std::string server_address("0.0.0.0:50051");

        ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        
        // 注册异步服务
        builder.RegisterService(&service_);
        
        // 创建 N 个完成队列 (Completion Queues)
        const int NUM_GRPC_CQ = 4; // 使用 4 个 CQ 来提高调度效率
        for (int i = 0; i < NUM_GRPC_CQ; ++i) {
            cqs_.emplace_back(builder.AddCompletionQueue());
        }

        server_ = builder.BuildAndStart();
        std::cout << "Server listening on " << server_address << std::endl;

        // 启动 gRPC I/O 线程来监听和调度事件
        for (int i = 0; i < NUM_GRPC_CQ; ++i) {
            // 每个 CQ 启动一个专门的调度线程
            grpc_threads_.emplace_back(
                [this, i] { HandleRpcs(cqs_[i].get()); }
            );
        }

        // 阻塞主线程，等待所有 gRPC I/O 线程完成
        for (auto& thread : grpc_threads_) {
            thread.join();
        }
    }

private:
    // 封装 RPC 调用的状态机
    class CallData {
    public:
        CallData(Greeter::AsyncService* service, CompletionQueue* cq, ThreadPool& pool)
            : service_(service), cq_(cq), responder_(&ctx_), status_(CREATE), business_thread_pool_(pool) {
            // 一旦创建，立即启动第一个 RPC 监听 (RequestSayHello)
            Proceed();
        }

        void Proceed() {
            if (status_ == CREATE) {
                // 状态机第一步：等待新请求
                status_ = PROCESS;
                service_->RequestSayHello(&ctx_, &request_, &responder_, cq_, cq_, this);
            } else if (status_ == PROCESS) {
                // 状态机第二步：收到请求，启动新 CallData 实例并提交业务任务
                new CallData(service_, cq_, business_thread_pool_); // 必须创建新的实例以监听下一个请求

                // 将耗时业务提交到独立的业务线程池
                business_thread_pool_.enqueue([this]() {
                    // **在这里执行耗时业务逻辑**
                    std::cout << "Business thread received: " << request_.name() << std::endl;
                    // 模拟耗时 500ms
                    std::this_thread::sleep_for(std::chrono::milliseconds(500)); 

                    std::string prefix("Async Hello ");
                    reply_.set_message(prefix + request_.name());

                    // 耗时任务完成，进入 FINISH 状态，并通知 gRPC I/O 线程发送回复
                    status_ = FINISH; 
                    responder_.Finish(reply_, Status::OK, this); 
                });
            } else { // status_ == FINISH
                // 状态机第三步：发送回复完毕，可以安全删除对象
                GPR_ASSERT(status_ == FINISH);
                delete this;
            }
        }

    private:
        Greeter::AsyncService* service_;
        CompletionQueue* cq_;
        ServerContext ctx_;
        HelloRequest request_;
        HelloReply reply_;
        ServerAsyncResponseWriter<HelloReply> responder_;
        ThreadPool& business_thread_pool_;
        
        // 状态机枚举
        enum CallStatus { CREATE, PROCESS, FINISH };
        CallStatus status_; // 当前 RPC 的状态
    };

    // gRPC I/O 线程的循环函数
    void HandleRpcs(CompletionQueue* cq) {
        // 监听第一个请求
        new CallData(&service_, cq, business_thread_pool_);
        void* tag; 
        bool ok;
        while (cq->Next(&tag, &ok)) { // 阻塞等待下一个事件
            CallData* call = static_cast<CallData*>(tag);
            GPR_ASSERT(ok);

            // 事件到达，继续处理状态机
            call->Proceed(); 
        }
    }

    std::unique_ptr<Server> server_;
    Greeter::AsyncService service_;
    std::vector<std::unique_ptr<CompletionQueue>> cqs_;
    std::vector<std::thread> grpc_threads_;
    ThreadPool business_thread_pool_;
};

void RunServer() {
    AsyncGreeterServer server(8); // 启动一个包含 8 个业务线程的线程池
    server.Run();
}

int main() {
    RunServer();
    return 0;
}