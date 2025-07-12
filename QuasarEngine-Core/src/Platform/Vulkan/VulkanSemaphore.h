#pragma once

#include "VulkanTypes.h"

namespace QuasarEngine
{
    class VulkanSemaphore {
    public:
        VulkanSemaphore(VkDevice device);
        ~VulkanSemaphore();

        VulkanSemaphore(const VulkanSemaphore&) = delete;
        VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

        VkDevice device;
        VkSemaphore handle;        
    };
}