#include "qepch.h"

#include "VulkanSemaphore.h"
#include "VulkanContext.h"

namespace QuasarEngine
{
    VulkanSemaphore::VulkanSemaphore(VkDevice device)
        : device(device)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo{};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, VulkanContext::Context.allocator->GetCallbacks(), &handle));
    }

    VulkanSemaphore::~VulkanSemaphore()
    {
        if (handle)
        {
            vkDestroySemaphore(device, handle, VulkanContext::Context.allocator->GetCallbacks());
            handle = VK_NULL_HANDLE;
        }
    }
}