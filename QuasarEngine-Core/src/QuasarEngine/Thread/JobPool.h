#pragma once

#include "Job.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#ifdef _WIN32
#   define NOMINMAX
#   include <Windows.h>
#endif

namespace QuasarEngine
{
    class JobPool
    {
    public:
        using JobFinishedCallback = std::function<void(const Job::Ptr&)>;

        JobPool(JobPoolType type,
            std::size_t threadCount,
            JobFinishedCallback onFinished = {},
            std::vector<int> coreAffinity = {});

        ~JobPool();

        JobPool(const JobPool&) = delete;
        JobPool& operator=(const JobPool&) = delete;
        JobPool(JobPool&&) = delete;
        JobPool& operator=(JobPool&&) = delete;

        JobPoolType GetType() const noexcept { return m_type; }

        void Enqueue(Job::Ptr job);

        void Shutdown();

    private:
        void WorkerLoop(std::size_t workerIndex);

        JobPoolType m_type;
        std::vector<std::thread> m_workers;

        std::priority_queue<
            Job::Ptr,
            std::vector<Job::Ptr>,
            JobPriorityComparator> m_jobs;

        std::mutex              m_queueMutex;
        std::condition_variable m_condition;
        std::atomic<bool>       m_stop{ false };

        std::vector<int>        m_affinity;
        JobFinishedCallback     m_onFinished;
    };

    inline JobPool::JobPool(JobPoolType type,
        std::size_t threadCount,
        JobFinishedCallback onFinished,
        std::vector<int> coreAffinity)
        : m_type(type)
        , m_affinity(std::move(coreAffinity))
        , m_onFinished(std::move(onFinished))
    {
        if (threadCount == 0)
            threadCount = 1;

        m_workers.reserve(threadCount);
        for (std::size_t i = 0; i < threadCount; ++i)
        {
            m_workers.emplace_back([this, i]() { WorkerLoop(i); });
        }
    }

    inline JobPool::~JobPool()
    {
        Shutdown();
    }

    inline void JobPool::Shutdown()
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

    inline void JobPool::Enqueue(Job::Ptr job)
    {
        if (!job)
            return;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            if (m_stop.load(std::memory_order_acquire))
            {
                return;
            }

            m_jobs.push(std::move(job));
        }
        m_condition.notify_one();
    }

    inline void JobPool::WorkerLoop(std::size_t workerIndex)
    {
#ifdef _WIN32
        if (!m_affinity.empty())
        {
            DWORD_PTR mask = 1ull << m_affinity[workerIndex % m_affinity.size()];
            SetThreadAffinityMask(GetCurrentThread(), mask);
        }
#else
        (void)workerIndex;
#endif

        for (;;)
        {
            Job::Ptr job;

            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_condition.wait(lock, [this]() {
                    return m_stop.load(std::memory_order_acquire)
                        || !m_jobs.empty();
                    });

                if (m_stop.load(std::memory_order_acquire) && m_jobs.empty())
                    return;

                job = m_jobs.top();
                m_jobs.pop();
            }

            if (job)
            {
                job->started.store(true, std::memory_order_release);

                try
                {
                    job->func();
                }
                catch (...)
                {
                    
                }

                job->finished.store(true, std::memory_order_release);

                if (m_onFinished)
                {
                    m_onFinished(job);
                }
            }
        }
    }
}