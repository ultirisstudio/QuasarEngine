#pragma once

#include "MemoryTracker.h"

namespace QuasarEngine
{
    inline void* alignedAlloc(std::size_t alignment, std::size_t size) {
#if defined(_MSC_VER) || defined(_WIN32)
        void* ptr = _aligned_malloc(size, alignment);
        auto& tracker = MemoryTracker::instance();
        tracker.Allocate(size);
        if (!ptr) return nullptr;
        return ptr;
#elif defined(__APPLE__) || defined(__unix__)
        void* ptr = nullptr;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return nullptr;
        }
        return ptr;
#else
#if defined(__cpp_aligned_new)
        return std::aligned_alloc(alignment, size);
#else
#error "No aligned allocation function available on this platform"
#endif
#endif
    }

    inline void alignedFree(void* ptr, std::size_t size) {
#if defined(_MSC_VER) || defined(_WIN32)
        _aligned_free(ptr);
#else
        free(ptr);
#endif

        auto& tracker = MemoryTracker::instance();
        tracker.Free(size);
    }
}