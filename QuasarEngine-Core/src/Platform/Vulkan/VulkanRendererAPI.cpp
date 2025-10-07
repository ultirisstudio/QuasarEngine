#include "qepch.h"
#include "VulkanRendererAPI.h"

#include "QuasarEngine/Renderer/DrawMode.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanShader.h"

namespace QuasarEngine
{
	namespace Utils
	{
		VkPrimitiveTopology ToVulkanPrimitiveTopology(DrawMode mode)
		{
			switch (mode)
			{
			case DrawMode::TRIANGLES:       return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			case DrawMode::TRIANGLE_STRIP:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			case DrawMode::TRIANGLE_FAN:    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
			case DrawMode::LINES:           return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case DrawMode::LINE_STRIP:      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			case DrawMode::LINE_LOOP:       Q_ERROR("LINE_LOOP is not supported in Vulkan nativement"); return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			case DrawMode::POINTS:          return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			default:                        Q_ERROR("Unknown DrawMode"); return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			}
		}
	}

	void VulkanRendererAPI::Initialize()
	{
		
	}

	void VulkanRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
			return;

		VkViewport viewport = {};
		viewport.x = static_cast<float>(x);
		viewport.y = static_cast<float>(y + height);
		viewport.width = static_cast<float>(width);
		viewport.height = static_cast<float>(-static_cast<int32_t>(height));
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(
			VulkanContext::Context.frameCommandBuffers.back()->handle,
			0, 1, &viewport
		);

		VkRect2D scissor = {};
		scissor.offset = { static_cast<int32_t>(x), static_cast<int32_t>(y) };
		scissor.extent = { width, height };

		vkCmdSetScissor(
			VulkanContext::Context.frameCommandBuffers.back()->handle,
			0, 1, &scissor
		);
	}

	void VulkanRendererAPI::ClearColor(const glm::vec4& color)
	{
		
	}

	void VulkanRendererAPI::Clear()
	{
		
	}

	void VulkanRendererAPI::DrawArrays(DrawMode drawMode, uint32_t size)
	{
		
	}

	void VulkanRendererAPI::DrawElements(DrawMode drawMode, uint32_t count, uint32_t firstIndex, int32_t baseVertex)
	{
		vkCmdDrawIndexed(VulkanContext::Context.frameCommandBuffers.back()->handle, count, 1, firstIndex, 0, 0);
	}
}