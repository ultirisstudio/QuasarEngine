#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace QuasarEngine
{
    class ThreadPool
    {
    public:
        explicit ThreadPool(unsigned int numThreads = std::thread::hardware_concurrency());
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        template<typename Func, typename... Args>
        auto Enqueue(Func&& f, Args&&... args)
            -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>;

        void Shutdown();

    private:
        void WorkerLoop();

        std::vector<std::thread>        m_workers;
        std::queue<std::function<void()>> m_tasks;

        std::mutex              m_mutex;
        std::condition_variable m_condition;
        std::atomic<bool>       m_stop{ false };
    };

    inline ThreadPool::ThreadPool(unsigned int numThreads)
    {
        if (numThreads == 0)
            numThreads = 1;

        m_workers.reserve(numThreads);
        for (unsigned int i = 0; i < numThreads; ++i)
        {
            m_workers.emplace_back([this]() { WorkerLoop(); });
        }
    }

    inline ThreadPool::~ThreadPool()
    {
        Shutdown();
    }

    inline void ThreadPool::Shutdown()
    {
        bool expected = false;
        if (!m_stop.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel))
        {
            return;
        }

        m_condition.notify_all();

        for (auto& worker : m_workers)
        {
            if (worker.joinable())
                worker.join();
        }
    }

    inline void ThreadPool::WorkerLoop()
    {
        for (;;)
        {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [this]() {
                    return m_stop.load(std::memory_order_acquire)
                        || !m_tasks.empty();
                    });

                if (m_stop.load(std::memory_order_acquire) && m_tasks.empty())
                    return;

                task = std::move(m_tasks.front());
                m_tasks.pop();
            }

            task();
        }
    }

    template<typename Func, typename... Args>
    auto ThreadPool::Enqueue(Func&& f, Args&&... args)
        -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>
    {
        using ReturnType = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> future = task->get_future();

        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_stop.load(std::memory_order_acquire))
                throw std::runtime_error("ThreadPool::Enqueue sur pool stoppé");

            m_tasks.emplace([task]() { (*task)(); });
        }

        m_condition.notify_one();
        return future;
    }
}