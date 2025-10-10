#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
#include <chrono>
#include <atomic>
#include <memory>

class SimpleThreadPool
{
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop{false};

public:
    SimpleThreadPool(size_t threads)
    {
        for (size_t i = 0; i < threads; ++i)
        {
            workers.emplace_back([this]
                                 {
                for (;;) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                } });
        }
    }

    ~SimpleThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }
    
    SimpleThreadPool(SimpleThreadPool const &) = delete;
    SimpleThreadPool(SimpleThreadPool &&) = delete;
    SimpleThreadPool &operator=(SimpleThreadPool const &) = delete;
    SimpleThreadPool &operator=(SimpleThreadPool &&) = delete;
    SimpleThreadPool() = delete;

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args)
        // -> std::future<typename std::result_of<F(Args...)>::type>
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([task]()
                          { (*task)(); });
        }
        condition.notify_one();
        return res;
    }
};

int main()
{
    SimpleThreadPool pool(4);

    auto result1 = pool.enqueue([](int a, int b)
                                {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return a + b; },
                                5, 3);

    auto result2 = pool.enqueue([](const std::string &str)
                                {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return str + " World"; },
                                "Hello");

    std::cout << "Result1: " << result1.get() << std::endl; // Outputs: 8
    std::cout << "Result2: " << result2.get() << std::endl; // Outputs: Hello World

    return 0; 
}