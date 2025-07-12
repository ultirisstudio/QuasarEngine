#pragma once

#include "VulkanTypes.h"

namespace QuasarEngine
{
	class VulkanFence
	{
	public:
		VulkanFence(VkDevice device, bool createSignal);
		VulkanFence(const VulkanFence&) = delete;
		VulkanFence& operator=(const VulkanFence&) = delete;

		VulkanFence(VulkanFence&&) = default;
		VulkanFence& operator=(VulkanFence&&) = default;

		~VulkanFence();

		bool Wait(uint64_t timeoutNS);
		void Reset();

		VkFence handle;

		VkDevice device;

		bool isSignaled;

	};
}