#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace QuasarEngine
{
    enum class JobPriority { LOW = 0, NORMAL, HIGH };
    enum class JobPoolType { GENERAL, IO, RENDER, SIMULATION };

    struct Job
    {
        using Ptr = std::shared_ptr<Job>;

        std::string        name;
        JobPriority        priority{ JobPriority::NORMAL };
        JobPoolType        pool{ JobPoolType::GENERAL };
        std::function<void()> func;

        std::vector<std::weak_ptr<Job>> dependencies;

        std::atomic<bool> started{ false };
        std::atomic<bool> finished{ false };

        Job() = default;

        template <typename F>
        Job(F&& f,
            JobPriority p = JobPriority::NORMAL,
            JobPoolType poolType = JobPoolType::GENERAL,
            std::string n = {})
            : name(std::move(n))
            , priority(p)
            , pool(poolType)
            , func(std::forward<F>(f))
        {
        }

        bool DependenciesFinished() const
        {
            for (auto& depWeak : dependencies)
            {
                if (auto dep = depWeak.lock())
                {
                    if (!dep->finished.load(std::memory_order_acquire))
                        return false;
                }
            }
            return true;
        }
    };

    struct JobPriorityComparator
    {
        bool operator()(const Job::Ptr& a, const Job::Ptr& b) const noexcept
        {
            return static_cast<int>(a->priority) < static_cast<int>(b->priority);
        }
    };
}