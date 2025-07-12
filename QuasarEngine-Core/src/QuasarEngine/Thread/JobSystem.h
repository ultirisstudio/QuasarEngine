#pragma once

#include "JobPool.h"

#include <QuasarEngine/Core/Singleton.h>

namespace QuasarEngine
{
    class JobSystem : public Singleton<JobSystem> {
    public:
        JobSystem() {
            pools[JobPoolType::GENERAL] = std::make_unique<JobPool>(JobPoolType::GENERAL, 2, std::vector<int>{0, 1});
            pools[JobPoolType::IO] = std::make_unique<JobPool>(JobPoolType::IO, 2, std::vector<int>{2, 3});
            pools[JobPoolType::RENDER] = std::make_unique<JobPool>(JobPoolType::RENDER, 1, std::vector<int>{4});
            pools[JobPoolType::SIMULATION] = std::make_unique<JobPool>(JobPoolType::SIMULATION, 1, std::vector<int>{5});
            StartDeadlockDetection();
        }

        ~JobSystem() {
            deadlockDetector.join();
        }

        template<typename Func, typename... Args>
        auto Submit(Func&& f, JobPriority pri, JobPoolType pool, std::vector<Job::Ptr> deps = {}, std::string name = "")
            -> std::future<typename std::invoke_result_t<Func, Args...>>
        {
            using ReturnType = typename std::invoke_result_t<Func, Args...>;

            auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<Func>(f));
            std::future<ReturnType> future = task->get_future();

            auto job = std::make_shared<Job>();
            job->func = [task]() { (*task)(); };
            job->priority = pri;
            job->poolType = pool;
            for (auto& d : deps) job->dependencies.push_back(d);
            job->name = name;

            pools[pool]->Submit(job);

            {
                std::lock_guard<std::mutex> lock(activeJobsMutex);
                activeJobs.insert(job);
            }

            return future;
        }

    private:
        std::map<JobPoolType, std::unique_ptr<JobPool>> pools;

        std::set<Job::Ptr> activeJobs;
        std::mutex activeJobsMutex;
        std::thread deadlockDetector;
        void StartDeadlockDetection() {
            deadlockDetector = std::thread([this]() {
                while (true) {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    std::lock_guard<std::mutex> lock(activeJobsMutex);
                    for (auto& job : activeJobs) {
                        if (!job->started && job->DependenciesReady()) {
                            std::cerr << "[DEADLOCK WARNING] Job " << job->name << " semble bloqué." << std::endl;
                        }
                    }
                }
                });
        }
    };
}