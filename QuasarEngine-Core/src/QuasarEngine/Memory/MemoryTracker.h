#pragma once

#include <QuasarEngine/Core/Singleton.h>

#include <vector>
#include <cstdlib>
#include <mutex>

namespace QuasarEngine
{
    class MemoryTracker : public Singleton<MemoryTracker> {
    public:
        void Allocate(size_t size);
        void Free(size_t size);
        void Update(float deltaTime);
        void Reset();

        size_t GetTotalAllocated() const;
        size_t GetTotalFreed() const;
        size_t GetCurrentUsage() const;
        const std::vector<float>& GetHistory() const;

        friend class Singleton<MemoryTracker>;
    private:
        MemoryTracker() = default;

        size_t totalAllocated = 0;
        size_t totalFreed = 0;
        size_t currentUsage = 0;

        std::vector<float> usageHistory;
        float maxTracked = 120.0f;
        float currentTime = 0.0f;

        std::chrono::steady_clock::time_point lastSampleTime = std::chrono::steady_clock::now();

        std::mutex mutex;
    };
}