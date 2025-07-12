#include "qepch.h"
#include "VulkanRenderPass.h"

#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "VulkanDevice.h"

#include "VulkanCommandBuffer.h"

namespace QuasarEngine
{
	VulkanRenderPass::VulkanRenderPass(VkDevice device, float x, float y, float w, float h, float r, float g, float b, float a, float depth,
		uint32_t stencil,
		VkSurfaceFormatKHR imageFormat,
		VkFormat depthFormat,
		VkImageLayout colorInitialLayout,
		VkImageLayout colorFinalLayout,
		VkImageLayout depthInitialLayout,
		VkImageLayout depthFinalLayout)
		: device(device), x(x), y(y), w(w), h(h), r(r), g(g), b(b), a(a), depth(depth), stencil(stencil)
	{
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

		const uint32_t attachment_descriptor_count = 2;
		VkAttachmentDescription attachment_descriptions[attachment_descriptor_count];

		VkAttachmentDescription color_attachment;
		color_attachment.format = imageFormat.format;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = colorInitialLayout;
		color_attachment.finalLayout = colorFinalLayout;
		color_attachment.flags = 0;

		attachment_descriptions[0] = color_attachment;

		VkAttachmentReference color_attachment_reference;
		color_attachment_reference.attachment = 0;
		color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_reference;

		VkAttachmentDescription depth_attachment;
		depth_attachment.format = depthFormat;
		depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_attachment.initialLayout = depthInitialLayout;
		depth_attachment.finalLayout = depthFinalLayout;
		depth_attachment.flags = 0;

		attachment_descriptions[1] = depth_attachment;

		VkAttachmentReference depth_attachment_reference;
		depth_attachment_reference.attachment = 1;
		depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		subpass.pDepthStencilAttachment = &depth_attachment_reference;

		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = 0;

		subpass.pResolveAttachments = 0;

		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = 0;

		VkSubpassDependency dependency;
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dependencyFlags = 0;

		VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		createInfo.attachmentCount = attachment_descriptor_count;
		createInfo.pAttachments = attachment_descriptions;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;
		createInfo.pNext = 0;
		createInfo.flags = 0;

		VK_CHECK(vkCreateRenderPass(device, &createInfo, VulkanContext::Context.allocator->GetCallbacks(), &renderpass));

		Q_DEBUG("Vulkan renderPass created successfully");
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		if (renderpass)
		{
			Q_DEBUG("Destroying Vulkan renderpass...");
			vkDestroyRenderPass(device, renderpass, VulkanContext::Context.allocator->GetCallbacks());
			renderpass = VK_NULL_HANDLE;
		}
	}

	void VulkanRenderPass::Begin(VulkanCommandBuffer* commandBuffer, VkFramebuffer framebuffer)
	{
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = renderpass;
		beginInfo.framebuffer = framebuffer;
		beginInfo.renderArea.offset.x = x;
		beginInfo.renderArea.offset.y = y;
		beginInfo.renderArea.extent.width = w;
		beginInfo.renderArea.extent.height = h;

		VkClearValue clearValues[2];
		clearValues[0].color.float32[0] = r;
		clearValues[0].color.float32[1] = g;
		clearValues[0].color.float32[2] = b;
		clearValues[0].color.float32[3] = a;
		clearValues[1].depthStencil.depth = depth;
		clearValues[1].depthStencil.stencil = stencil;

		beginInfo.clearValueCount = 2;
		beginInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffer->handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		commandBuffer->SetState(VulkanCommandBuffer::COMMAND_BUFFER_STATE_IN_RENDER_PASS);
	}

	void VulkanRenderPass::End(VulkanCommandBuffer* commandBuffer)
	{
		vkCmdEndRenderPass(commandBuffer->handle);
		commandBuffer->SetState(VulkanCommandBuffer::COMMAND_BUFFER_STATE_RECORDING);
	}
}