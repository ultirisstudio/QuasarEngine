#pragma once

#include "VulkanTypes.h"

namespace QuasarEngine
{
	class VulkanCommandBuffer
	{
	public:
		VulkanCommandBuffer(VkDevice device, VkCommandPool pool);
		VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
		VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;

		VulkanCommandBuffer(VulkanCommandBuffer&&) = default;
		VulkanCommandBuffer& operator=(VulkanCommandBuffer&&) = default;

		~VulkanCommandBuffer();

		void Allocate(bool isPrimary);
		void Free();

		void Begin(bool isSingleUse, bool isRenderPassContinue, bool isSimultaneousUse);
		void End();

		void UpdateSubmitted();

		void Reset();

		void AllocateAndBeginSingleUse();
		void EndSingleUse(VkQueue queue);

		enum vulkanCommandBufferState : uint8_t
		{
			COMMAND_BUFFER_STATE_READY,
			COMMAND_BUFFER_STATE_RECORDING,
			COMMAND_BUFFER_STATE_IN_RENDER_PASS,
			COMMAND_BUFFER_STATE_RECORDING_ENDED,
			COMMAND_BUFFER_STATE_SUBMITTED,
			COMMAND_BUFFER_STATE_NOT_ALLOCATED
		};

		void SetState(vulkanCommandBufferState newState);

		VkCommandBuffer handle;

		VkDevice device;
		VkCommandPool pool;

		vulkanCommandBufferState state;
		
	};
}