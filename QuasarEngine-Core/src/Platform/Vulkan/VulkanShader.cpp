#include "qepch.h"

#include "VulkanContext.h"
#include "VulkanShader.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanTexture2D.h"
#include "VulkanImage.h"

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Core/Window.h>

#include <glm/glm.hpp>

#include "VulkanFramebuffer.h"

#define INVALID_ID 4294967295U

#define MAX_OBJECTS 4000

namespace QuasarEngine
{
	std::vector<uint32_t> ReadSPIRV(const std::string& filepath)
	{
		std::ifstream file(filepath, std::ios::ate | std::ios::binary);
		if (!file.is_open()) throw std::runtime_error("Failed to open shader file!");

		size_t filesize = (size_t)file.tellg();
		std::vector<uint32_t> buffer(filesize / sizeof(uint32_t));
		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.data()), filesize);
		file.close();
		return buffer;
	}

	VkShaderStageFlagBits ToVkShaderStage(Shader::ShaderStageType stage)
	{
		switch (stage) {
		case Shader::ShaderStageType::Vertex:      return VK_SHADER_STAGE_VERTEX_BIT;
		case Shader::ShaderStageType::Fragment:    return VK_SHADER_STAGE_FRAGMENT_BIT;
		case Shader::ShaderStageType::Geometry:    return VK_SHADER_STAGE_GEOMETRY_BIT;
		case Shader::ShaderStageType::Compute:     return VK_SHADER_STAGE_COMPUTE_BIT;
		case Shader::ShaderStageType::TessControl: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case Shader::ShaderStageType::TessEval:    return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		default: throw std::runtime_error("Unknown ShaderStageType");
		}
	}

	inline VkShaderStageFlags ShaderStageFlagsToVk(uint32_t stageFlags)
	{
		VkShaderStageFlags vkFlags = 0;
		if (stageFlags & Shader::StageToBit(Shader::ShaderStageType::Vertex))      vkFlags |= VK_SHADER_STAGE_VERTEX_BIT;
		if (stageFlags & Shader::StageToBit(Shader::ShaderStageType::Fragment))    vkFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
		if (stageFlags & Shader::StageToBit(Shader::ShaderStageType::Geometry))    vkFlags |= VK_SHADER_STAGE_GEOMETRY_BIT;
		if (stageFlags & Shader::StageToBit(Shader::ShaderStageType::Compute))     vkFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
		if (stageFlags & Shader::StageToBit(Shader::ShaderStageType::TessControl)) vkFlags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		if (stageFlags & Shader::StageToBit(Shader::ShaderStageType::TessEval))    vkFlags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		return vkFlags;
	}

	inline VkFormat ShaderIOTypeToVkFormat(Shader::ShaderIOType type) {
		switch (type) {
		case Shader::ShaderIOType::Float:  return VK_FORMAT_R32_SFLOAT;
		case Shader::ShaderIOType::Vec2:   return VK_FORMAT_R32G32_SFLOAT;
		case Shader::ShaderIOType::Vec3:   return VK_FORMAT_R32G32B32_SFLOAT;
		case Shader::ShaderIOType::Vec4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Shader::ShaderIOType::Int:    return VK_FORMAT_R32_SINT;
		case Shader::ShaderIOType::IVec2:  return VK_FORMAT_R32G32_SINT;
		case Shader::ShaderIOType::IVec3:  return VK_FORMAT_R32G32B32_SINT;
		case Shader::ShaderIOType::IVec4:  return VK_FORMAT_R32G32B32A32_SINT;
		case Shader::ShaderIOType::UInt:   return VK_FORMAT_R32_UINT;
		case Shader::ShaderIOType::UVec2:  return VK_FORMAT_R32G32_UINT;
		case Shader::ShaderIOType::UVec3:  return VK_FORMAT_R32G32B32_UINT;
		case Shader::ShaderIOType::UVec4:  return VK_FORMAT_R32G32B32A32_UINT;
		default: return VK_FORMAT_UNDEFINED;
		}
	}

	inline uint32_t ShaderIOTypeToSize(Shader::ShaderIOType type) {
		switch (type) {
		case Shader::ShaderIOType::Float:  return 4;
		case Shader::ShaderIOType::Vec2:   return 4 * 2;
		case Shader::ShaderIOType::Vec3:   return 4 * 3;
		case Shader::ShaderIOType::Vec4:   return 4 * 4;
		case Shader::ShaderIOType::Int:    return 4;
		case Shader::ShaderIOType::IVec2:  return 4 * 2;
		case Shader::ShaderIOType::IVec3:  return 4 * 3;
		case Shader::ShaderIOType::IVec4:  return 4 * 4;
		case Shader::ShaderIOType::UInt:   return 4;
		case Shader::ShaderIOType::UVec2:  return 4 * 2;
		case Shader::ShaderIOType::UVec3:  return 4 * 3;
		case Shader::ShaderIOType::UVec4:  return 4 * 4;
		default: return 0;
		}
	}

	inline size_t AlignUBOOffset(size_t offset, size_t alignment) {
		return (offset + alignment - 1) & ~(alignment - 1);
	}

	inline VkDescriptorType ShaderUniformTypeToVk(Shader::ShaderUniformType type)
	{
		return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	}

	inline VkDescriptorType ShaderSamplerTypeToVk()
	{
		return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	}

	inline VkCullModeFlags ToVkCullMode(Shader::CullMode mode) {
		switch (mode) {
		case Shader::CullMode::None: return VK_CULL_MODE_NONE;
		case Shader::CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
		case Shader::CullMode::Back: return VK_CULL_MODE_BACK_BIT;
		case Shader::CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
		}
		return VK_CULL_MODE_BACK_BIT;
	}

	inline VkPolygonMode ToVkPolygonMode(Shader::FillMode mode) {
		switch (mode) {
		case Shader::FillMode::Solid: return VK_POLYGON_MODE_FILL;
		case Shader::FillMode::Wireframe: return VK_POLYGON_MODE_LINE;
		}
		return VK_POLYGON_MODE_FILL;
	}

	inline VkCompareOp ToVkCompareOp(Shader::DepthFunc func) {
		switch (func) {
		case Shader::DepthFunc::Never: return VK_COMPARE_OP_NEVER;
		case Shader::DepthFunc::Less: return VK_COMPARE_OP_LESS;
		case Shader::DepthFunc::Equal: return VK_COMPARE_OP_EQUAL;
		case Shader::DepthFunc::LessOrEqual: return VK_COMPARE_OP_LESS_OR_EQUAL;
		case Shader::DepthFunc::Greater: return VK_COMPARE_OP_GREATER;
		case Shader::DepthFunc::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
		case Shader::DepthFunc::GreaterOrEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case Shader::DepthFunc::Always: return VK_COMPARE_OP_ALWAYS;
		}
		return VK_COMPARE_OP_LESS;
	}

	inline VkPrimitiveTopology ToVkPrimitiveTopology(Shader::PrimitiveTopology topo) {
		switch (topo) {
		case Shader::PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case Shader::PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case Shader::PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		}
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}

	inline PipelineBlendMode ToPipelineBlendMode(Shader::BlendMode blend) {
		switch (blend) {
		case Shader::BlendMode::None:       return PipelineBlendMode::None;
		case Shader::BlendMode::AlphaBlend: return PipelineBlendMode::AlphaBlend;
		case Shader::BlendMode::Additive:   return PipelineBlendMode::Additive;
		default:                    return PipelineBlendMode::None;
		}
	}

	inline PipelineCullMode ToPipelineCullMode(Shader::CullMode mode) {
		switch (mode) {
		case Shader::CullMode::None:         return PipelineCullMode::None;
		case Shader::CullMode::Front:        return PipelineCullMode::Front;
		case Shader::CullMode::Back:         return PipelineCullMode::Back;
		case Shader::CullMode::FrontAndBack: return PipelineCullMode::FrontAndBack;
		default:                             return PipelineCullMode::Back;
		}
	}

	inline PipelineFillMode ToPipelineFillMode(Shader::FillMode mode) {
		switch (mode) {
		case Shader::FillMode::Solid:     return PipelineFillMode::Solid;
		case Shader::FillMode::Wireframe: return PipelineFillMode::Wireframe;
		default:                          return PipelineFillMode::Solid;
		}
	}

	VulkanShader::VulkanShader(const ShaderDescription& desc) : c_offset(0), objectUniformBufferIndex(0), m_Description(desc)
	{
		for (const auto& uniform : m_Description.globalUniforms) {
			m_GlobalUniformMap[uniform.name] = &uniform;
		}
		
		for (const auto& uniform : m_Description.objectUniforms) {
			m_ObjectUniformMap[uniform.name] = &uniform;
		}

		m_Stages = CreateShaderStages(VulkanContext::Context.device->device);

		Q_DEBUG("Vulkan shader initialized successfully");

		// GLOBAL UBO

		std::vector<VkDescriptorSetLayoutBinding> globalBindings;

		if (!m_Description.globalUniforms.empty()) {
			const auto& uniform = m_Description.globalUniforms[0];
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = uniform.binding;
			layoutBinding.descriptorCount = 1;
			layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			layoutBinding.pImmutableSamplers = nullptr;

			ShaderStageFlags stages = 0;
			for (const auto& u : m_Description.globalUniforms)
				stages |= u.stages;
			layoutBinding.stageFlags = ShaderStageFlagsToVk(stages);

			globalBindings.push_back(layoutBinding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(globalBindings.size());
		layoutInfo.pBindings = globalBindings.data();

		VK_CHECK(vkCreateDescriptorSetLayout(
			VulkanContext::Context.device->device,
			&layoutInfo,
			VulkanContext::Context.allocator->GetCallbacks(),
			&globalDescriptorSetLayout
		));

		// Pool
		VkDescriptorPoolSize global_pool_size{};
		global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		global_pool_size.descriptorCount = VulkanContext::Context.swapchain->images.size() * globalBindings.size();

		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &global_pool_size;
		poolInfo.maxSets = VulkanContext::Context.swapchain->images.size();

		VK_CHECK(vkCreateDescriptorPool(
			VulkanContext::Context.device->device,
			&poolInfo,
			VulkanContext::Context.allocator->GetCallbacks(),
			&globalDescriptorPool
		));

		// OBJECTS UBO

		std::vector<VkDescriptorSetLayoutBinding> objectBindings;

		if (!m_Description.objectUniforms.empty()) {
			const auto& uniform = m_Description.objectUniforms[0];
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = uniform.binding;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			binding.descriptorCount = 1;
			ShaderStageFlags stages = 0;
			for (const auto& u : m_Description.objectUniforms)
				stages |= u.stages;
			binding.stageFlags = ShaderStageFlagsToVk(stages);
			binding.pImmutableSamplers = nullptr;
			objectBindings.push_back(binding);
		}

		for (const auto& sampler : m_Description.samplers) {
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = sampler.binding;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.descriptorCount = 1;
			binding.stageFlags = ShaderStageFlagsToVk(sampler.stages);
			binding.pImmutableSamplers = nullptr;
			objectBindings.push_back(binding);
		}

		VkDescriptorSetLayoutCreateInfo objectLayoutInfo{};
		objectLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		objectLayoutInfo.bindingCount = static_cast<uint32_t>(objectBindings.size());
		objectLayoutInfo.pBindings = objectBindings.data();

		VK_CHECK(vkCreateDescriptorSetLayout(
			VulkanContext::Context.device->device,
			&objectLayoutInfo,
			VulkanContext::Context.allocator->GetCallbacks(),
			&objectDescriptorSetLayout
		));

		uint32_t swapchainImageCount = VulkanContext::Context.swapchain->images.size();
		uint32_t objectUBOCount = static_cast<uint32_t>(m_Description.objectUniforms.size());
		uint32_t samplerCount = static_cast<uint32_t>(m_Description.samplers.size());

		std::vector<VkDescriptorPoolSize> objectPoolSizes;

		if (objectUBOCount > 0) {
			VkDescriptorPoolSize uboPoolSize{};
			uboPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboPoolSize.descriptorCount = objectUBOCount * MAX_OBJECTS * swapchainImageCount;
			objectPoolSizes.push_back(uboPoolSize);
		}
		if (samplerCount > 0) {
			VkDescriptorPoolSize samplerPoolSize{};
			samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerPoolSize.descriptorCount = samplerCount * MAX_OBJECTS * swapchainImageCount;
			objectPoolSizes.push_back(samplerPoolSize);
		}

		VkDescriptorPoolCreateInfo objectPoolInfo{};
		objectPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		objectPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		objectPoolInfo.poolSizeCount = static_cast<uint32_t>(objectPoolSizes.size());
		objectPoolInfo.pPoolSizes = objectPoolSizes.data();
		objectPoolInfo.maxSets = MAX_OBJECTS * swapchainImageCount;

		VK_CHECK(vkCreateDescriptorPool(
			VulkanContext::Context.device->device,
			&objectPoolInfo,
			VulkanContext::Context.allocator->GetCallbacks(),
			&objectDescriptorPool
		));

		//

		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = VulkanContext::Context.height;
		viewport.width = VulkanContext::Context.width;
		viewport.height = -VulkanContext::Context.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor;
		scissor.offset.x = scissor.offset.y = 0;
		scissor.extent.width = VulkanContext::Context.width;
		scissor.extent.height = VulkanContext::Context.height;

		// Attributes

		std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
		uint32_t offset = 0;

		const auto& vertexInputs = desc.modules[0].inputs;

		for (const auto& input : vertexInputs) {
			VkVertexInputAttributeDescription attr{};
			attr.location = input.location;
			attr.binding = 0;
			attr.format = ShaderIOTypeToVkFormat(input.type);
			attr.offset = offset;
			attributeDescriptions.push_back(attr);

			offset += ShaderIOTypeToSize(input.type);
		}
		
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		descriptorSetLayouts.push_back(globalDescriptorSetLayout);
		descriptorSetLayouts.push_back(objectDescriptorSetLayout);
		
		std::vector<VkPipelineShaderStageCreateInfo> stage_create_infos;
		stage_create_infos.resize(m_Stages.size());

		for (uint32_t i = 0; i < m_Stages.size(); i++)
		{
			stage_create_infos[i] = m_Stages[i].stage_info;
		}

		std::vector<VkPushConstantRange> pushConstants;
		for (const auto& pc : desc.pushConstants)
		{
			VkPushConstantRange range{};
			range.stageFlags = ShaderStageFlagsToVk(pc.stages);
			range.offset = pc.offset;
			range.size = pc.size;
			pushConstants.push_back(range);
		}

		VulkanPipelineDesc pipelineDesc;
		pipelineDesc.renderPass = m_Description.framebuffer ? dynamic_cast<VulkanFramebuffer*>(m_Description.framebuffer)->GetRenderPass()->renderpass : VulkanContext::Context.mainRenderPass->renderpass;
		pipelineDesc.attributes = attributeDescriptions;
		pipelineDesc.descriptorSetLayouts = descriptorSetLayouts;
		pipelineDesc.stages = stage_create_infos;
		pipelineDesc.pushConstants = pushConstants;
		pipelineDesc.stride = offset;

		pipelineDesc.viewport = viewport;
		pipelineDesc.scissor = scissor;

		pipelineDesc.cullMode = ToPipelineCullMode(m_Description.cullMode);
		pipelineDesc.fillMode = ToPipelineFillMode(m_Description.fillMode);
		pipelineDesc.blendMode = ToPipelineBlendMode(m_Description.blendMode);

		pipelineDesc.depthTestEnable = m_Description.depthTestEnable;
		pipelineDesc.depthWriteEnable = m_Description.depthWriteEnable;
		pipelineDesc.depthCompareOp = ToVkCompareOp(m_Description.depthFunc);

		pipelineDesc.enableDynamicViewport = m_Description.enableDynamicViewport;
		pipelineDesc.enableDynamicScissor = m_Description.enableDynamicScissor;
		pipelineDesc.enableDynamicLineWidth = m_Description.enableDynamicLineWidth;

		pipeline = std::make_unique<VulkanPipeline>(pipelineDesc);

		size_t globalUBOSize = 0;
		for (const auto& uniform : m_Description.globalUniforms)
			globalUBOSize = std::max(globalUBOSize, uniform.offset + uniform.size);
		m_GlobalUniformData.resize(globalUBOSize);

		uint32_t device_local_bits = VulkanContext::Context.device->supportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
		globalUniformBuffer = std::make_unique<VulkanBuffer>(VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			globalUBOSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | device_local_bits,
			true);

		std::vector<VkDescriptorSetLayout> global_layouts;
		global_layouts.resize(VulkanContext::Context.swapchain->images.size(), globalDescriptorSetLayout);

		VkDescriptorSetAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		alloc_info.descriptorPool = globalDescriptorPool;
		alloc_info.descriptorSetCount = VulkanContext::Context.swapchain->images.size();
		alloc_info.pSetLayouts = global_layouts.data();

		globalDescriptorSets.resize(VulkanContext::Context.swapchain->images.size());
		VK_CHECK(vkAllocateDescriptorSets(VulkanContext::Context.device->device, &alloc_info, globalDescriptorSets.data()));

		size_t objectUBOSize = 0;
		for (const auto& uniform : m_Description.objectUniforms)
			objectUBOSize = std::max(objectUBOSize, uniform.offset + uniform.size);
		m_ObjectUniformData.resize(objectUBOSize);

		objectUniformBuffer = std::make_unique<VulkanBuffer>(VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			objectUBOSize * MAX_OBJECTS,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			true);

		TextureSpecification spec;
		spec.width = 2;
		spec.height = 2;
		spec.format = TextureFormat::RGBA;
		spec.internal_format = TextureFormat::RGBA;
		spec.compressed = false;
		spec.alpha = true;
		spec.flip = false;
		spec.wrap_s = TextureWrap::REPEAT;
		spec.wrap_t = TextureWrap::REPEAT;
		spec.wrap_r = TextureWrap::REPEAT;
		spec.min_filter_param = TextureFilter::NEAREST;
		spec.mag_filter_param = TextureFilter::NEAREST;

		std::vector<unsigned char> bluePixels(4 * spec.width * spec.height, 0);
		for (int i = 0; i < spec.width * spec.height; ++i) {
			bluePixels[i * 4 + 0] = 255;
			bluePixels[i * 4 + 1] = 0;
			bluePixels[i * 4 + 2] = 0;
			bluePixels[i * 4 + 3] = 255;
		}
		defaultBlueTexture = new VulkanTexture2D(spec);
		defaultBlueTexture->LoadFromData(bluePixels.data(), bluePixels.size());

		Q_ASSERT(defaultBlueTexture != nullptr, "defaultBlueTexture nullptr");
		Q_ASSERT(defaultBlueTexture->sampler != VK_NULL_HANDLE, "defaultBlueTexture->sampler nullptr");
		Q_ASSERT(defaultBlueTexture->image != nullptr, "defaultBlueTexture->image nullptr");
		Q_ASSERT(defaultBlueTexture->image->view != VK_NULL_HANDLE, "defaultBlueTexture->image->view nullptr");
	}

	VulkanShader::~VulkanShader()
	{
		delete defaultBlueTexture;

		objectUniformBuffer.reset();
		globalUniformBuffer.reset();

		pipeline.reset();

		VkDevice device = VulkanContext::Context.device->device;

		if (objectDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device, objectDescriptorPool, VulkanContext::Context.allocator->GetCallbacks());
			objectDescriptorPool = VK_NULL_HANDLE;
		}

		if (objectDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device, objectDescriptorSetLayout, VulkanContext::Context.allocator->GetCallbacks());
			objectDescriptorSetLayout = VK_NULL_HANDLE;
		}

		if (globalDescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(device, globalDescriptorPool, VulkanContext::Context.allocator->GetCallbacks());
			globalDescriptorPool = VK_NULL_HANDLE;
		}

		if (globalDescriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(device, globalDescriptorSetLayout, VulkanContext::Context.allocator->GetCallbacks());
			globalDescriptorSetLayout = VK_NULL_HANDLE;
		}

		for (size_t i = 0; i < m_Stages.size(); ++i)
		{
			if (m_Stages[i].handle != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(device, m_Stages[i].handle, VulkanContext::Context.allocator->GetCallbacks());
				m_Stages[i].handle = VK_NULL_HANDLE;
			}
		}
		m_Stages.clear();

		objectStates.clear();
		globalDescriptorSets.clear();

		objectUniformBufferIndex = 0;
		c_offset = 0;
	}

	std::vector<VulkanShaderStage> VulkanShader::CreateShaderStages(VkDevice device)
	{
		std::vector<VulkanShaderStage> stages;
		stages.reserve(m_Description.modules.size());

		for (const auto& moduleInfo : m_Description.modules)
		{
			VulkanShaderStage stage;
			stage.stage_type = moduleInfo.stage;
			stage.path = moduleInfo.path;

			auto spirv = ReadSPIRV(moduleInfo.path);

			stage.handle = CreateVkShaderModule(device, spirv);

			stage.stage_info = {};
			stage.stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.stage_info.stage = ToVkShaderStage(moduleInfo.stage);
			stage.stage_info.module = stage.handle;
			stage.stage_info.pName = "main";

			stages.push_back(std::move(stage));
		}
		return stages;
	}

	VkShaderModule VulkanShader::CreateVkShaderModule(VkDevice device, const std::vector<uint32_t>& code)
	{
		VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.codeSize = code.size() * sizeof(uint32_t);
		createInfo.pCode = code.data();

		VkShaderModule module;
		if (vkCreateShaderModule(device, &createInfo, VulkanContext::Context.allocator->GetCallbacks(), &module) != VK_SUCCESS)
			throw std::runtime_error("Failed to create shader module!");
		return module;
	}

	bool VulkanShader::UpdateGlobalState()
	{
		uint32_t imageIndex = VulkanContext::Context.imageIndex;
		VkDescriptorSet globalDescriptor = globalDescriptorSets[imageIndex];

		if (!globalUniformBuffer || globalUniformBuffer->handle == VK_NULL_HANDLE) {
			Q_ERROR("globalUniformBuffer handle is VK_NULL_HANDLE!");
			return false;
		}

		globalUniformBuffer->LoadData(0, m_GlobalUniformData.size(), 0, m_GlobalUniformData.data());

		VkDescriptorBufferInfo buffer_info;
		buffer_info.buffer = globalUniformBuffer->handle;
		buffer_info.offset = 0;
		buffer_info.range = m_GlobalUniformData.size();

		VkWriteDescriptorSet descriptor_write = {};
		descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_write.dstSet = globalDescriptor;
		descriptor_write.dstBinding = 0;
		descriptor_write.dstArrayElement = 0;
		descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_write.descriptorCount = 1;
		descriptor_write.pBufferInfo = &buffer_info;

		vkUpdateDescriptorSets(VulkanContext::Context.device->device, 1, &descriptor_write, 0, nullptr);

		vkCmdBindDescriptorSets(
			VulkanContext::Context.frameCommandBuffers.back()->handle,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->layout,
			0,
			1, &globalDescriptor,
			0, nullptr
		);

		return true;
	}

	bool VulkanShader::UpdateObject(Material* material)
	{
		if (material->m_ID >= MAX_OBJECTS) {
			Q_ERROR("Object ID exceeds maximum limit");
			return false;
		}
		if (objectStates.size() <= material->m_ID) {
			Q_ERROR("Object ID is out of range");
			return false;
		}
		if (!objectUniformBuffer || objectUniformBuffer->handle == VK_NULL_HANDLE) {
			Q_ERROR("objectUniformBuffer handle is VK_NULL_HANDLE!");
			return false;
		}

		ObjectShaderObjectState* objectState = &objectStates[material->m_ID];
		VkDescriptorSet objectDescriptorSet = objectState->descriptorSets[VulkanContext::Context.imageIndex];
		if (objectDescriptorSet == VK_NULL_HANDLE) {
			Q_ERROR("objectDescriptorSet is VK_NULL_HANDLE!");
			return false;
		}

		size_t minAlignment = VulkanContext::Context.device->physicalDeviceInfo.limits.minUniformBufferOffsetAlignment;
		size_t structSize = m_ObjectUniformData.size();
		size_t alignedStructSize = AlignUBOOffset(structSize, minAlignment);

		size_t bufferOffset = material->m_ID * alignedStructSize;

		objectUniformBuffer->LoadData(bufferOffset, structSize, 0, m_ObjectUniformData.data());

		VkDescriptorBufferInfo buffer_info;
		buffer_info.buffer = objectUniformBuffer->handle;
		buffer_info.offset = bufferOffset;
		buffer_info.range = structSize;

		std::vector<VkWriteDescriptorSet> descriptorWrites;

		VkWriteDescriptorSet uboWrite{};
		uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		uboWrite.dstSet = objectDescriptorSet;
		uboWrite.dstBinding = 0;
		uboWrite.dstArrayElement = 0;
		uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboWrite.descriptorCount = 1;
		uboWrite.pBufferInfo = &buffer_info;
		descriptorWrites.push_back(uboWrite);

		std::vector<VkDescriptorImageInfo> imageInfos(m_Description.samplers.size());
		std::vector<VkWriteDescriptorSet> samplerWrites;

		for (size_t i = 0; i < m_Description.samplers.size(); ++i)
		{
			const auto& samplerDesc = m_Description.samplers[i];
			VulkanTexture2D* tex = nullptr;

			auto it = m_ObjectTextures.find(samplerDesc.name);
			if (it != m_ObjectTextures.end() && it->second)
				tex = it->second;

			if (!tex)
				tex = defaultBlueTexture;

			if (!tex || !tex->image || tex->image->view == VK_NULL_HANDLE || tex->sampler == VK_NULL_HANDLE)
			{
				Q_ERROR("Texture ou sampler invalide pour '%s'. (tex=%p, image=%p, view=%p, sampler=%p)",
					samplerDesc.name.c_str(),
					tex, tex ? tex->image : nullptr, tex && tex->image ? (void*)tex->image->view : nullptr, tex ? (void*)tex->sampler : nullptr);
				continue;
			}

			imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfos[i].imageView = tex->image->view;
			imageInfos[i].sampler = tex->sampler;

			VkWriteDescriptorSet samplerWrite{};
			samplerWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			samplerWrite.dstSet = objectDescriptorSet;
			samplerWrite.dstBinding = samplerDesc.binding;
			samplerWrite.dstArrayElement = 0;
			samplerWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerWrite.descriptorCount = 1;
			samplerWrite.pImageInfo = &imageInfos[i];
			samplerWrites.push_back(samplerWrite);
		}

		descriptorWrites.insert(descriptorWrites.end(), samplerWrites.begin(), samplerWrites.end());

		if (!descriptorWrites.empty()) {
			vkUpdateDescriptorSets(
				VulkanContext::Context.device->device,
				static_cast<uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(),
				0, nullptr
			);
		}

		vkCmdBindDescriptorSets(
			VulkanContext::Context.frameCommandBuffers.back()->handle,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->layout,
			1,
			1, &objectDescriptorSet,
			0, nullptr
		);

		return true;
	}

	void VulkanShader::PushConstant(const void* data, size_t size, VkShaderStageFlags stageFlags, uint32_t offset)
	{
		vkCmdPushConstants(
			VulkanContext::Context.frameCommandBuffers.back()->handle,
			pipeline->layout,
			stageFlags,
			offset,
			static_cast<uint32_t>(size),
			data
		);
	}

	void VulkanShader::Use()
	{
		pipeline->Bind(VulkanContext::Context.frameCommandBuffers.back()->handle, VK_PIPELINE_BIND_POINT_GRAPHICS);
	}

	void VulkanShader::Unuse()
	{		
		c_offset = 0;
	}

	void VulkanShader::Reset()
	{
		c_offset = 0;
	}

	bool VulkanShader::AcquireResources(Material* material)
	{
		ObjectShaderObjectState object_state;

		uint32_t swapchainImageCount = VulkanContext::Context.swapchain->images.size();

		uint32_t bindingCount = static_cast<uint32_t>(
			m_Description.objectUniforms.size() + m_Description.samplers.size()
			);

		object_state.descriptorStates.resize(bindingCount);
		for (uint32_t i = 0; i < bindingCount; ++i) {
			for (uint32_t j = 0; j < swapchainImageCount; ++j) {
				object_state.descriptorStates[i].generations[j] = INVALID_ID;
				object_state.descriptorStates[i].ids[j] = INVALID_ID;
			}
		}

		std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, objectDescriptorSetLayout);

		VkDescriptorSetAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = objectDescriptorPool;
		alloc_info.descriptorSetCount = swapchainImageCount;
		alloc_info.pSetLayouts = layouts.data();

		object_state.descriptorSets.resize(swapchainImageCount);

		VkResult result = vkAllocateDescriptorSets(
			VulkanContext::Context.device->device,
			&alloc_info,
			object_state.descriptorSets.data()
		);

		if (result != VK_SUCCESS) {
			Q_ERROR("Error allocating descriptor sets in shader");
			return false;
		}

		material->m_ID = objectUniformBufferIndex;
		objectStates.push_back(std::move(object_state));
		objectUniformBufferIndex++;

		return true;
	}
	
	void VulkanShader::ReleaseResources(Material* material)
	{
		vkDeviceWaitIdle(VulkanContext::Context.device->device);

		if (material->m_ID == INVALID_ID || material->m_ID >= objectStates.size())
			return;

		ObjectShaderObjectState* objectState = &objectStates[material->m_ID];

		VkResult result = vkFreeDescriptorSets(
			VulkanContext::Context.device->device,
			objectDescriptorPool,
			static_cast<uint32_t>(objectState->descriptorSets.size()),
			objectState->descriptorSets.data()
		);
		if (result != VK_SUCCESS) {
			Q_ERROR("Error freeing object shader descriptor sets");
		}

		for (auto& ds : objectState->descriptorStates) {
			for (uint32_t& g : ds.generations) g = INVALID_ID;
			for (uint32_t& id : ds.ids) id = INVALID_ID;
		}

		material->m_ID = INVALID_ID;
	}

	void VulkanShader::SetUniform(const std::string& name, void* data, size_t size)
	{
		const ShaderUniformDesc* desc = nullptr;
		auto it = m_GlobalUniformMap.find(name);
		if (it != m_GlobalUniformMap.end()) {
			desc = it->second;
			if (size != desc->size) {
				Q_ERROR("Uniform '%s' size mismatch (got %zu, expected %zu)", name.c_str(), size, desc->size);
				return;
			}
			std::memcpy(m_GlobalUniformData.data() + desc->offset, data, size);
			return;
		}
		it = m_ObjectUniformMap.find(name);
		if (it != m_ObjectUniformMap.end()) {
			desc = it->second;
			if (size != desc->size) {
				Q_ERROR("Uniform '%s' size mismatch (got %zu, expected %zu)", name.c_str(), size, desc->size);
				return;
			}
			std::memcpy(m_ObjectUniformData.data() + desc->offset, data, size);
			return;
		}
		Q_ERROR("Uniform '%s' not found", name.c_str());
	}

	void VulkanShader::SetTexture(const std::string& name, Texture* texture, SamplerType type)
	{
		auto it = std::find_if(
			m_Description.samplers.begin(), m_Description.samplers.end(),
			[&](const ShaderSamplerDesc& desc) { return desc.name == name; });

		if (it == m_Description.samplers.end()) {
			Q_ERROR("Sampler '%s' not found in shader description!", name.c_str());
			return;
		}

		m_ObjectTextures[name] = texture ? dynamic_cast<VulkanTexture2D*>(texture) : defaultBlueTexture;
	}

	//PushConstant(&value, sizeof(glm::mat4), VK_SHADER_STAGE_VERTEX_BIT, c_offset);
	//c_offset += sizeof(glm::mat4);
}