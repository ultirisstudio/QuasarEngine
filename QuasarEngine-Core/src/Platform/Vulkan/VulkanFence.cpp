#include "qepch.h"
#include "VulkanFence.h"

#include "VulkanContext.h"

namespace QuasarEngine
{
	VulkanFence::VulkanFence(VkDevice device, bool createSignal)
		: device(device), isSignaled(createSignal)
	{
		VkFenceCreateInfo fence_create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		if (isSignaled)
		{
			fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		}

		VK_CHECK(vkCreateFence(device, &fence_create_info, VulkanContext::Context.allocator->GetCallbacks(), &handle));
	}

	VulkanFence::~VulkanFence()
	{
		if (handle)
		{
			vkDestroyFence(device, handle, VulkanContext::Context.allocator->GetCallbacks());
			handle = nullptr;
		}

		isSignaled = false;
	}

	bool VulkanFence::Wait(uint64_t timeoutNS)
	{
		if (!isSignaled)
		{
			VkResult result = vkWaitForFences(device, 1, &handle, true, timeoutNS);

			switch (result)
			{
			case VK_SUCCESS:
				isSignaled = true;
				return true;
			case VK_TIMEOUT:
				Q_WARNING("Fence Wait - Time out");
				break;
			case VK_ERROR_DEVICE_LOST:
				Q_ERROR("Fence Wait - VK_ERROR_DEVICE_LOST");
				break;
			case VK_ERROR_OUT_OF_HOST_MEMORY:
				Q_ERROR("Fence Wait - VK_ERROR_OUT_OF_HOST_MEMORY");
				break;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
				Q_ERROR("Fence Wait - VK_ERROR_OUT_OF_DEVICE_MEMORY");
				break;
			default:
				Q_ERROR("Fence Wait - An unknown error has occured");
				break;
			}
		}
		else
		{
			return true;
		}

		return false;
	}

	void VulkanFence::Reset()
	{
		if (isSignaled)
		{
			VK_CHECK(vkResetFences(device, 1, &handle));
			isSignaled = false;
		}
	}
}