#pragma once

#include "VulkanTypes.h"

namespace QuasarEngine
{
    enum class PipelineCullMode {
        None,
        Front,
        Back,
        FrontAndBack
    };

    enum class PipelineFillMode {
        Solid,
        Wireframe
    };

    enum class PipelineBlendMode {
        None,
        AlphaBlend,
        Additive
    };

    struct VulkanPipelineDesc {
        VkRenderPass renderPass;
        std::vector<VkVertexInputAttributeDescription> attributes;
        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        std::vector<VkPipelineShaderStageCreateInfo> stages;
        std::vector<VkPushConstantRange> pushConstants;
        uint32_t stride;

        VkViewport viewport;
        VkRect2D scissor;

        PipelineCullMode cullMode = PipelineCullMode::Back;
        PipelineFillMode fillMode = PipelineFillMode::Solid;
        PipelineBlendMode blendMode = PipelineBlendMode::None;

        VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;
        bool depthWriteEnable = true;
        bool depthTestEnable = true;

        bool enableDynamicViewport = true;
        bool enableDynamicScissor = true;
        bool enableDynamicLineWidth = false;
    };

	class VulkanPipeline
	{
	public:
		VulkanPipeline(const VulkanPipelineDesc& desc);
		~VulkanPipeline();

		VulkanPipeline(const VulkanPipeline&) = delete;
		VulkanPipeline& operator=(const VulkanPipeline&) = delete;

		void Bind(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint);

		VkPipeline handle;
		VkPipelineLayout layout;

        VulkanPipelineDesc description;
	};
}