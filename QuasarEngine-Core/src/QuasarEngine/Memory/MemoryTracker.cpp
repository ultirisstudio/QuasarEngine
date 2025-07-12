#include "qepch.h"
#include "MemoryTracker.h"

namespace QuasarEngine
{
    void MemoryTracker::Allocate(size_t size) {
        std::lock_guard<std::mutex> lock(mutex);
        totalAllocated += size;
        currentUsage += size;
    }

    void MemoryTracker::Free(size_t size) {
        std::lock_guard<std::mutex> lock(mutex);
        totalFreed += size;
        currentUsage -= size;
    }

    void MemoryTracker::Update(float deltaTime) {
        auto now = std::chrono::steady_clock::now();
        float delta = std::chrono::duration<float>(now - lastSampleTime).count();


        std::lock_guard<std::mutex> lock(mutex);
        currentTime += deltaTime;

        if (delta >= 1.0f) {
            if (usageHistory.size() >= static_cast<size_t>(maxTracked))
                usageHistory.erase(usageHistory.begin());
            usageHistory.push_back(static_cast<float>(currentUsage) / (1024.0f * 1024.0f));

            lastSampleTime = now;
        }
    }

    void MemoryTracker::Reset() {
        std::lock_guard<std::mutex> lock(mutex);
        totalAllocated = totalFreed = currentUsage = 0;
        usageHistory.clear();
    }

    size_t MemoryTracker::GetTotalAllocated() const {
        return totalAllocated;
    }

    size_t MemoryTracker::GetTotalFreed() const {
        return totalFreed;
    }

    size_t MemoryTracker::GetCurrentUsage() const {
        return currentUsage;
    }

    const std::vector<float>& MemoryTracker::GetHistory() const {
        return usageHistory;
    }
}