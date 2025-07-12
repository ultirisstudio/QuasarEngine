#include "qepch.h"

#include "VulkanPipeline.h"
#include "VulkanContext.h"

#include <glm/glm.hpp>

#include "VulkanDevice.h"

namespace QuasarEngine
{
	VkCullModeFlags ToVkCullMode(PipelineCullMode mode) {
		switch (mode) {
		case PipelineCullMode::None:         return VK_CULL_MODE_NONE;
		case PipelineCullMode::Front:        return VK_CULL_MODE_FRONT_BIT;
		case PipelineCullMode::Back:         return VK_CULL_MODE_BACK_BIT;
		case PipelineCullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
		}
		return VK_CULL_MODE_BACK_BIT;
	}

	VkPolygonMode ToVkPolygonMode(PipelineFillMode mode) {
		switch (mode) {
		case PipelineFillMode::Solid:     return VK_POLYGON_MODE_FILL;
		case PipelineFillMode::Wireframe: return VK_POLYGON_MODE_LINE;
		}
		return VK_POLYGON_MODE_FILL;
	}

	VkPipelineColorBlendAttachmentState MakeVkBlendState(PipelineBlendMode mode) {
		VkPipelineColorBlendAttachmentState state = {};
		state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		switch (mode) {
		case PipelineBlendMode::None:
			state.blendEnable = VK_FALSE;
			break;
		case PipelineBlendMode::AlphaBlend:
			state.blendEnable = VK_TRUE;
			state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			state.colorBlendOp = VK_BLEND_OP_ADD;
			state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			state.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		case PipelineBlendMode::Additive:
			state.blendEnable = VK_TRUE;
			state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
			state.colorBlendOp = VK_BLEND_OP_ADD;
			state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			state.alphaBlendOp = VK_BLEND_OP_ADD;
			break;
		}
		return state;
	}

    VulkanPipeline::VulkanPipeline(const VulkanPipelineDesc& desc)
        : handle(VK_NULL_HANDLE), layout(VK_NULL_HANDLE)
    {
        std::vector<VkDynamicState> dynamicStates;
        if (desc.enableDynamicViewport) dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
        if (desc.enableDynamicScissor) dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
        if (desc.enableDynamicLineWidth) dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicStateCreateInfo.dynamicStateCount = dynamicStates.size();
        dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.pViewports = &desc.viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &desc.scissor;

        VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizerCreateInfo.depthClampEnable = VK_FALSE;
        rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizerCreateInfo.polygonMode = ToVkPolygonMode(desc.fillMode);
        rasterizerCreateInfo.lineWidth = 1.0f;
        rasterizerCreateInfo.cullMode = ToVkCullMode(desc.cullMode);
        rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = desc.depthTestEnable ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = desc.depthWriteEnable ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = desc.depthCompareOp;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        auto blendAttachment = MakeVkBlendState(desc.blendMode);

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &blendAttachment;

        VkVertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = desc.stride;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = desc.attributes.size();
        vertexInputInfo.pVertexAttributeDescriptions = desc.attributes.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutCreateInfo.setLayoutCount = desc.descriptorSetLayouts.size();
        pipelineLayoutCreateInfo.pSetLayouts = desc.descriptorSetLayouts.data();
        pipelineLayoutCreateInfo.pushConstantRangeCount = desc.pushConstants.size();
        pipelineLayoutCreateInfo.pPushConstantRanges = desc.pushConstants.data();

        VK_CHECK(vkCreatePipelineLayout(VulkanContext::Context.device->device, &pipelineLayoutCreateInfo, VulkanContext::Context.allocator->GetCallbacks(), &layout));

        VkGraphicsPipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineCreateInfo.stageCount = desc.stages.size();
        pipelineCreateInfo.pStages = desc.stages.data();
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
        pipelineCreateInfo.pViewportState = &viewportState;
        pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencil;
        pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.layout = layout;
        pipelineCreateInfo.renderPass = desc.renderPass;
        pipelineCreateInfo.subpass = 0;

        VkResult result = vkCreateGraphicsPipelines(VulkanContext::Context.device->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VulkanContext::Context.allocator->GetCallbacks(), &handle);

        if (!VulkanResultIsSuccess(result)) {
            std::string str = "vkCreateGraphicsPipelines failed with result : " + VulkanResultString(result);
            Q_ERROR(str);
            return;
        }

        Q_DEBUG("Vulkan pipeline created successfully");
    }

	VulkanPipeline::~VulkanPipeline()
	{
		vkDeviceWaitIdle(VulkanContext::Context.device->device);

		if (handle)
		{
			vkDestroyPipeline(VulkanContext::Context.device->device, handle, VulkanContext::Context.allocator->GetCallbacks());
			handle = VK_NULL_HANDLE;
		}

		if (layout)
		{
			vkDestroyPipelineLayout(VulkanContext::Context.device->device, layout, VulkanContext::Context.allocator->GetCallbacks());
			layout = VK_NULL_HANDLE;
		}
	}

	void VulkanPipeline::Bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint)
	{
		vkCmdBindPipeline(commandBuffer, bindPoint, handle);
	}
}
