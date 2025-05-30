#pragma once
#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace fallout {
class ThreadPool {
public:
    explicit ThreadPool(size_t count);
    ~ThreadPool();
    void enqueue(std::function<void()> fn);
    void wait();
private:
    std::vector<std::jthread> m_threads;
    std::queue<std::function<void()>> m_jobs;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_shutdown = false;
};
}
