#pragma once

#include <vulkan/vulkan.h>

namespace QuasarEngine
{
	class VulkanCommandBuffer;
	class VulkanFramebuffer;

	class VulkanRenderPass
	{
	public:
		VulkanRenderPass(VkDevice device, float x, float y, float w, float h, float r, float g, float b, float a, float depth,
			uint32_t stencil,
			VkSurfaceFormatKHR imageFormat,
			VkFormat depthFormat,
			VkImageLayout colorInitialLayout,
			VkImageLayout colorFinalLayout,
			VkImageLayout depthInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			VkImageLayout depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		~VulkanRenderPass();

		VulkanRenderPass(const VulkanRenderPass&) = delete;
		VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

		void Begin(VulkanCommandBuffer* commandBuffer, VkFramebuffer framebuffer);
		void End(VulkanCommandBuffer* commandBuffer);

		void SetX(float newX) { x = newX; }
		void SetY(float newY) { y = newY; }
		void SetWidth(float width) { w = width; }
		void SetHeight(float height) { h = height; }

		enum vulkanRenderPassState : uint8_t
		{
			READY,
			RECORDING,
			IN_RENDER_PASS,
			RECORDING_ENDED,
			SUBMITTED,
			NOT_ALLOCATED
		};

		VkDevice device;

		VkRenderPass renderpass;

		float x, y, w, h;
		float r, g, b, a;

		float depth;
		uint32_t stencil;

		vulkanRenderPassState state;
	};
}