#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace QuasarEngine
{
    class ThreadPool
    {
    public:
        explicit ThreadPool(unsigned numThreads = std::thread::hardware_concurrency())
            : m_Stop(false)
        {
            if (numThreads == 0)
                numThreads = 1;

            for (unsigned i = 0; i < numThreads; ++i)
            {
                m_Workers.emplace_back([this]() { WorkerLoop(); });
            }
        }

        ~ThreadPool()
        {
            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_Stop = true;
            }
            m_Condition.notify_all();

            for (auto& t : m_Workers)
                if (t.joinable())
                    t.join();
        }

        template<typename F>
        void Enqueue(F&& func)
        {
            {
                std::unique_lock<std::mutex> lock(m_QueueMutex);
                m_Tasks.emplace(std::forward<F>(func));
            }
            m_Condition.notify_one();
        }

    private:
        void WorkerLoop()
        {
            while (true)
            {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(m_QueueMutex);
                    m_Condition.wait(lock, [this]() {
                        return m_Stop || !m_Tasks.empty();
                        });

                    if (m_Stop && m_Tasks.empty())
                        return;

                    task = std::move(m_Tasks.front());
                    m_Tasks.pop();
                }

                task();
            }
        }

    private:
        std::vector<std::thread> m_Workers;
        std::queue<std::function<void()>> m_Tasks;

        std::mutex m_QueueMutex;
        std::condition_variable m_Condition;
        bool m_Stop;
    };

}