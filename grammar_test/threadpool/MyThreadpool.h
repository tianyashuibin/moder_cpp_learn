//
// Created by bee-eater on 2025/11/9.
//

#pragma once

#include <vector>
#include <thread>
#include <condition_variable>
#include <functional>
#include <mutex>

namespace MHY {
class MyThreadpool {
public:
    explicit MyThreadpool(size_t threads = std::thread::hardware_concurrency());
    ~MyThreadpool();

    // 任务入队函数：模板函数，必须在头文件中定义
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)->std::future<decltype(f(args...))>;

private:
    void worker_loop();
    std::vector<thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

template<class F, class... Args>
auto MyThreadpool::enqueue(F&& f, Args&&... args)->std::future<decltype(F(args...))> {
    using return_type = decltype(F(args...);

    auto task = std::make_shared<std::packaged_task<return_type()>> (
        std::bind(std::forward<F>(f), std::forward<Args>(arg)...)
    );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop) {
            throw std::runtime_error("enqueue on stopped threadpool");
        }

        tasks.emplace([task]() {
            (*task)();
        });
    }

    condition.notify_one();

    return res;
}

} // MHY
