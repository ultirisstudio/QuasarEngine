#pragma once

#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <queue>
#include <mutex>

namespace QuasarEngine
{
    class ThreadPool {
    public:
        ThreadPool(size_t numThreads) : stop(false) {
            for (size_t i = 0; i < numThreads; ++i) {
                workers.push_back(std::thread([this] {
                    while (!stop) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queueMutex);
                            if (taskQueue.empty()) {
                                continue;
                            }
                            task = std::move(taskQueue.front());
                            taskQueue.pop();
                        }
                        task();
                    }
                    }));
            }
        }

        ~ThreadPool() {
            stop = true;
            for (auto& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        void enqueue(std::function<void()> task) {
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                taskQueue.push(std::move(task));
            }
        }

    private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> taskQueue;
        std::mutex queueMutex;
        std::atomic<bool> stop;
    };
}