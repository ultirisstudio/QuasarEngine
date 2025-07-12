#include <queue>
#include <vector>
#include <functional>
#include <future>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <map>
#include <set>
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif

namespace QuasarEngine
{
    enum class JobPriority { LOW = 0, NORMAL, HIGH };
    enum class JobPoolType { GENERAL, IO, RENDER, SIMULATION };

    struct Job {
        using Ptr = std::shared_ptr<Job>;
        std::function<void()> func;
        JobPriority priority;
        JobPoolType poolType;
        std::vector<std::weak_ptr<Job>> dependencies;
        std::atomic<bool> started{ false };
        std::string name;

        bool DependenciesReady() const {
            for (auto& dep : dependencies) {
                if (auto sp = dep.lock()) {
                    if (!sp->started.load())
                        return false;
                }
            }
            return true;
        }
    };

    struct JobComparator {
        bool operator()(const Job::Ptr& a, const Job::Ptr& b) const {
            return static_cast<int>(a->priority) < static_cast<int>(b->priority);
        }
    };
}