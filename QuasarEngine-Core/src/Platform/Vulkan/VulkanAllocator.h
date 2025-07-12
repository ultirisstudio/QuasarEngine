#include <vulkan/vulkan.h>
#include <mutex>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <QuasarEngine/Memory/Allocator.h>

namespace QuasarEngine
{
    class VulkanAllocator
    {
    public:
        explicit VulkanAllocator(const std::string& name = "VulkanAllocator")
            : m_name(name)
        {
            m_callbacks.pUserData = this;
            m_callbacks.pfnAllocation = AllocFn;
            m_callbacks.pfnReallocation = ReallocFn;
            m_callbacks.pfnFree = FreeFn;
            m_callbacks.pfnInternalAllocation = InternalAllocNotificationFn;
            m_callbacks.pfnInternalFree = InternalFreeNotificationFn;
        }

        const VkAllocationCallbacks* GetCallbacks() const {
            return &m_callbacks;
        }

        size_t GetTotalAllocated() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_totalAllocated;
        }

    private:
        static void* VKAPI_PTR AllocFn(
            void* pUserData,
            size_t size,
            size_t alignment,
            VkSystemAllocationScope)
        {
            VulkanAllocator* self = static_cast<VulkanAllocator*>(pUserData);

            if (size == 0)
                return nullptr;

            void* ptr = alignedAlloc(alignment, size);
            if (!ptr) {
                return nullptr;
            }

            {
                std::lock_guard<std::mutex> lock(self->m_mutex);
                self->m_allocations[ptr] = size;
                self->m_totalAllocated += size;
            }

            return ptr;
        }

        static void* VKAPI_PTR ReallocFn(
            void* pUserData,
            void* pOriginal,
            size_t size,
            size_t alignment,
            VkSystemAllocationScope)
        {
            VulkanAllocator* self = static_cast<VulkanAllocator*>(pUserData);

            if (size == 0) {
                FreeFn(pUserData, pOriginal);
                return nullptr;
            }

            void* newPtr = alignedAlloc(alignment, size);
            if (!newPtr) {
                return nullptr;
            }

            {
                std::lock_guard<std::mutex> lock(self->m_mutex);
                auto it = self->m_allocations.find(pOriginal);
                if (it != self->m_allocations.end()) {
                    size_t oldSize = it->second;
                    std::memcpy(newPtr, pOriginal, std::min(oldSize, size));
                    self->m_allocations.erase(it);
                    self->m_totalAllocated -= oldSize;
                    alignedFree(pOriginal, oldSize);
                }
                self->m_allocations[newPtr] = size;
                self->m_totalAllocated += size;
            }

            return newPtr;
        }

        static void VKAPI_PTR FreeFn(void* pUserData, void* pMemory) {
            if (!pMemory) return;

            VulkanAllocator* self = static_cast<VulkanAllocator*>(pUserData);
            size_t size = 0;

            {
                std::lock_guard<std::mutex> lock(self->m_mutex);
                auto it = self->m_allocations.find(pMemory);
                if (it != self->m_allocations.end()) {
                    size = it->second;
                    self->m_allocations.erase(it);
                    self->m_totalAllocated -= size;
                }
            }

            alignedFree(pMemory, size);
        }

        static void VKAPI_PTR InternalAllocNotificationFn(
            void*,
            size_t size,
            VkInternalAllocationType allocationType,
            VkSystemAllocationScope)
        {
            std::cout << "[VulkanAllocator] Internal allocation of size " << size
                << " type " << allocationType << std::endl;
        }

        static void VKAPI_PTR InternalFreeNotificationFn(
            void*,
            size_t size,
            VkInternalAllocationType allocationType,
            VkSystemAllocationScope)
        {
            std::cout << "[VulkanAllocator] Internal free of size " << size
                << " type " << allocationType << std::endl;
        }

    private:
        std::string m_name;
        VkAllocationCallbacks m_callbacks;

        mutable std::mutex m_mutex;
        std::unordered_map<void*, size_t> m_allocations;
        size_t m_totalAllocated = 0;
    };
}