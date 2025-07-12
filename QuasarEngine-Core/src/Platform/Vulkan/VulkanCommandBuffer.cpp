#include "qepch.h"
#include "VulkanCommandBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"

namespace QuasarEngine
{
	VulkanCommandBuffer::VulkanCommandBuffer(VkDevice device, VkCommandPool pool)
		: handle(VK_NULL_HANDLE), device(device), pool(pool)
	{

	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
		if (handle != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(device, pool, 1, &handle);
			handle = VK_NULL_HANDLE;
			state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
		}
	}

	void VulkanCommandBuffer::Allocate(bool isPrimary)
	{
		VkCommandBufferAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocInfo.commandPool = pool;
		allocInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		allocInfo.commandBufferCount = 1;
		allocInfo.pNext = nullptr;

		state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
		VK_CHECK(vkAllocateCommandBuffers(device, &allocInfo, &handle));
		state = COMMAND_BUFFER_STATE_READY;
	}

	void VulkanCommandBuffer::Free()
	{
		vkFreeCommandBuffers(device, pool, 1, &handle);

		handle = VK_NULL_HANDLE;
		state = COMMAND_BUFFER_STATE_NOT_ALLOCATED;
	}

	void VulkanCommandBuffer::Begin(bool isSingleUse, bool isRenderPassContinue, bool isSimultaneousUse)
	{
		VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		beginInfo.flags = 0;
		if (isSingleUse)
		{
			beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		}
		if (isRenderPassContinue)
		{
			beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		}
		if (isSimultaneousUse)
		{
			beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		}

		VK_CHECK(vkBeginCommandBuffer(handle, &beginInfo));
		state = COMMAND_BUFFER_STATE_RECORDING;
	}

	void VulkanCommandBuffer::End()
	{
		VK_CHECK(vkEndCommandBuffer(handle));

		state = COMMAND_BUFFER_STATE_RECORDING_ENDED;
	}

	void VulkanCommandBuffer::UpdateSubmitted()
	{
		state = COMMAND_BUFFER_STATE_SUBMITTED;
	}

	void VulkanCommandBuffer::Reset()
	{
		VK_CHECK(vkResetCommandBuffer(handle, 0));

		state = COMMAND_BUFFER_STATE_READY;
	}

	void VulkanCommandBuffer::AllocateAndBeginSingleUse()
	{
		Allocate(true);
		Begin(true, false, false);
	}

	void VulkanCommandBuffer::EndSingleUse(VkQueue queue)
	{
		End();

		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &handle;

		VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, 0));

		VK_CHECK(vkQueueWaitIdle(queue));

		Free();
	}

	void VulkanCommandBuffer::SetState(vulkanCommandBufferState newState)
	{
		state = newState;
	}
}