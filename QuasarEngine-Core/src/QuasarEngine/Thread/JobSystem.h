#pragma once

#include "JobPool.h"

#include <QuasarEngine/Core/Singleton.h>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace QuasarEngine
{
    class JobSystem : public Singleton<JobSystem>
    {
    public:
        JobSystem();
        ~JobSystem();

        JobSystem(const JobSystem&) = delete;
        JobSystem& operator=(const JobSystem&) = delete;

        template<typename Func, typename... Args>
        auto Submit(JobPriority      priority,
            JobPoolType      pool,
            std::vector<Job::Ptr> dependencies,
            std::string      name,
            Func&& f,
            Args&&...        args)
            -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>;

        template<typename Func, typename... Args>
        auto Submit(JobPriority      priority,
            JobPoolType      pool,
            std::vector<Job::Ptr> dependencies,
            Func&& f,
            Args&&...        args)
            -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>
        {
            return Submit(priority,
                pool,
                std::move(dependencies),
                {},
                std::forward<Func>(f),
                std::forward<Args>(args)...);
        }

        template<typename Func, typename... Args>
        auto Submit(JobPriority priority,
            JobPoolType pool,
            Func&& f,
            Args&&...   args)
            -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>
        {
            return Submit(priority,
                pool,
                {},
                {},
                std::forward<Func>(f),
                std::forward<Args>(args)...);
        }

        template<typename Func, typename... Args>
        auto Submit(Func&& f, Args&&... args)
            -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>
        {
            return Submit(JobPriority::NORMAL,
                JobPoolType::GENERAL,
                {},
                {},
                std::forward<Func>(f),
                std::forward<Args>(args)...);
        }

        void WaitAll();

    private:
        friend class Singleton<JobSystem>;

        void InitDefaultPools();
        void OnJobFinished(const Job::Ptr& job);

        void StartDeadlockDetection();
        void StopDeadlockDetection();

        using JobSet = std::set<Job::Ptr>;

        std::map<JobPoolType, std::unique_ptr<JobPool>> m_pools;

        std::mutex m_jobsMutex;
        JobSet     m_allJobs;
        JobSet     m_pendingJobs;
        JobSet     m_activeJobs;

        std::atomic<bool> m_stop{ false };
        std::thread       m_deadlockDetector;
    };

    inline JobSystem::JobSystem()
    {
        InitDefaultPools();
        StartDeadlockDetection();
    }

    inline JobSystem::~JobSystem()
    {
        WaitAll();
        
        StopDeadlockDetection();

        for (auto& entry : m_pools)
        {
            if (entry.second)
                entry.second->Shutdown();
        }
    }

    inline void JobSystem::InitDefaultPools()
    {
        const unsigned hw = std::thread::hardware_concurrency();
        const std::size_t generalThreads = hw > 2 ? (hw - 2) : 1;
        const std::size_t ioThreads = 1;
        const std::size_t renderThreads = 1;
        const std::size_t simThreads = 1;

        auto makePool = [this](JobPoolType type,
            std::size_t threadCount)
            {
                m_pools[type] = std::make_unique<JobPool>(
                    type,
                    threadCount,
                    [this](const Job::Ptr& job) { OnJobFinished(job); },
                    std::vector<int>{}
                );
            };

        makePool(JobPoolType::GENERAL, generalThreads);
        makePool(JobPoolType::IO, ioThreads);
        makePool(JobPoolType::RENDER, renderThreads);
        makePool(JobPoolType::SIMULATION, simThreads);
    }

    inline void JobSystem::OnJobFinished(const Job::Ptr& job)
    {
        std::vector<Job::Ptr> nowReady;

        {
            std::lock_guard<std::mutex> lock(m_jobsMutex);

            m_activeJobs.erase(job);

            for (auto it = m_pendingJobs.begin(); it != m_pendingJobs.end(); )
            {
                const auto& pendingJob = *it;
                if (pendingJob->DependenciesFinished())
                {
                    nowReady.push_back(pendingJob);
                    it = m_pendingJobs.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }

        for (auto& ready : nowReady)
        {
            auto poolIt = m_pools.find(ready->pool);
            if (poolIt != m_pools.end())
            {
                poolIt->second->Enqueue(ready);
            }
        }
    }

    inline void JobSystem::StartDeadlockDetection()
    {
        m_stop.store(false, std::memory_order_release);

        m_deadlockDetector = std::thread([this]()
            {
                using namespace std::chrono_literals;

                while (!m_stop.load(std::memory_order_acquire))
                {
                    std::this_thread::sleep_for(5s);

                    std::vector<Job::Ptr> snapshot;
                    {
                        std::lock_guard<std::mutex> lock(m_jobsMutex);
                        snapshot.assign(m_activeJobs.begin(), m_activeJobs.end());
                    }

                    for (const auto& job : snapshot)
                    {
                        if (!job->started.load(std::memory_order_acquire) &&
                            !job->finished.load(std::memory_order_acquire) &&
                            job->DependenciesFinished())
                        {
                            std::cerr << "[JobSystem] Possible deadlock pour le job \""
                                << job->name << "\"\n";
                        }
                    }
                }
            });
    }

    inline void JobSystem::StopDeadlockDetection()
    {
        m_stop.store(true, std::memory_order_release);
        if (m_deadlockDetector.joinable())
            m_deadlockDetector.join();
    }

    inline void JobSystem::WaitAll()
    {
        for (;;)
        {
            bool anyActive = false;

            {
                std::lock_guard<std::mutex> lock(m_jobsMutex);
                for (const auto& job : m_allJobs)
                {
                    if (!job->finished.load(std::memory_order_acquire))
                    {
                        anyActive = true;
                        break;
                    }
                }
            }

            if (!anyActive)
                return;

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    template<typename Func, typename... Args>
    auto JobSystem::Submit(JobPriority      priority,
        JobPoolType      pool,
        std::vector<Job::Ptr> dependencies,
        std::string      name,
        Func&& f,
        Args&&...        args)
        -> std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>
    {
        using FunctionType = std::decay_t<Func>;
        using ReturnType = std::invoke_result_t<FunctionType, std::decay_t<Args>...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> future = task->get_future();

        auto job = std::make_shared<Job>();
        job->name = std::move(name);
        job->priority = priority;
        job->pool = pool;
        job->func = [task]() { (*task)(); };

        job->dependencies.clear();
        job->dependencies.reserve(dependencies.size());
        for (auto& dep : dependencies)
            job->dependencies.emplace_back(dep);

        {
            std::lock_guard<std::mutex> lock(m_jobsMutex);

            m_allJobs.insert(job);
            m_activeJobs.insert(job);

            if (job->DependenciesFinished())
            {
                auto it = m_pools.find(pool);
                if (it == m_pools.end())
                    throw std::runtime_error("JobSystem::Submit : pool inconnu");

                it->second->Enqueue(job);
            }
            else
            {
                m_pendingJobs.insert(job);
            }
        }

        return future;
    }
}