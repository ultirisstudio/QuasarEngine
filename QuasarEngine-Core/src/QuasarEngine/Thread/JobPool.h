#pragma once

#include "Job.h"

namespace QuasarEngine
{
    class JobPool {
    public:
        JobPool(JobPoolType type, size_t threadCount, std::vector<int> coreAffinity = {})
            : type(type), stopFlag(false), affinity(coreAffinity)
        {
            for (size_t i = 0; i < threadCount; ++i) {
                workers.emplace_back([this, i]() {
#ifdef _WIN32
                    if (!affinity.empty()) {
                        DWORD_PTR mask = 1ull << affinity[i % affinity.size()];
                        SetThreadAffinityMask(GetCurrentThread(), mask);
                    }
#endif

                    while (true) {
                        Job::Ptr job;
                        {
                            std::unique_lock<std::mutex> lock(queueMutex);
                            condition.wait(lock, [this]() { return stopFlag || !jobs.empty(); });
                            if (stopFlag && jobs.empty())
                                return;

                            for (auto it = jobs.rbegin(); it != jobs.rend(); ++it) {
                                if ((*it)->DependenciesReady()) {
                                    job = *it;
                                    jobs.erase(std::next(it).base());
                                    break;
                                }
                            }
                            if (!job) continue;
                        }
                        job->started = true;
                        try {
                            job->func();
                        }
                        catch (...) {
                            
                        }
                    }
                    });
            }
        }

        ~JobPool() {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                stopFlag = true;
            }
            condition.notify_all();
            for (auto& w : workers) w.join();
        }

        void Submit(Job::Ptr job) {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                jobs.push_back(job);
                std::sort(jobs.begin(), jobs.end(), JobComparator());
            }
            condition.notify_one();
        }

        std::vector<Job::Ptr> PendingJobs() {
            std::unique_lock<std::mutex> lock(queueMutex);
            return jobs;
        }

    private:
        JobPoolType type;
        std::vector<std::thread> workers;
        std::vector<Job::Ptr> jobs;
        std::vector<int> affinity;
        std::mutex queueMutex;
        std::condition_variable condition;
        std::atomic<bool> stopFlag;
    };
}