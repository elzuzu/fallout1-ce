#include "render/ThreadPool.h"

namespace fallout {

ThreadPool::ThreadPool(size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        m_threads.emplace_back([this] {
            for (;;) {
                std::function<void()> job;
                {
                    std::unique_lock<std::mutex> lock(m_mutex);
                    m_cv.wait(lock, [&]{ return m_shutdown || !m_jobs.empty(); });
                    if (m_shutdown && m_jobs.empty()) return;
                    job = std::move(m_jobs.front());
                    m_jobs.pop();
                }
                job();
            }
        });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shutdown = true;
    }
    m_cv.notify_all();
}

void ThreadPool::enqueue(std::function<void()> fn)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.push(std::move(fn));
    }
    m_cv.notify_one();
}

void ThreadPool::wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [&]{ return m_jobs.empty(); });
}

} // namespace fallout
