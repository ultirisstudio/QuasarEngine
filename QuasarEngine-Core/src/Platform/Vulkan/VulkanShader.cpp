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
#include "VulkanFramebuffer.h"

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Application.h>
#include <QuasarEngine/Core/Window.h>

#include <glm/glm.hpp>

#include <fstream>
#include <algorithm>
#include <cstring>

#define INVALID_ID 4294967295U
#define MAX_OBJECTS 4000

namespace QuasarEngine
{
    static std::vector<uint32_t> ReadSPIRV(const std::string& filepath)
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

    static VkShaderStageFlagBits ToVkShaderStage(Shader::ShaderStageType stage)
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

    static inline VkShaderStageFlags ShaderStageFlagsToVk(uint32_t stageFlags)
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

    static inline VkFormat ShaderIOTypeToVkFormat(Shader::ShaderIOType type) {
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

    static inline uint32_t ShaderIOTypeToSize(Shader::ShaderIOType type) {
        switch (type) {
        case Shader::ShaderIOType::Float:  return 4;
        case Shader::ShaderIOType::Vec2:   return 8;
        case Shader::ShaderIOType::Vec3:   return 12;
        case Shader::ShaderIOType::Vec4:   return 16;
        case Shader::ShaderIOType::Int:    return 4;
        case Shader::ShaderIOType::IVec2:  return 8;
        case Shader::ShaderIOType::IVec3:  return 12;
        case Shader::ShaderIOType::IVec4:  return 16;
        case Shader::ShaderIOType::UInt:   return 4;
        case Shader::ShaderIOType::UVec2:  return 8;
        case Shader::ShaderIOType::UVec3:  return 12;
        case Shader::ShaderIOType::UVec4:  return 16;
        default: return 0;
        }
    }

    static inline VkCompareOp ToVkCompareOp(Shader::DepthFunc func) {
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

    static inline PipelineBlendMode ToPipelineBlendMode(Shader::BlendMode blend) {
        switch (blend) {
        case Shader::BlendMode::None:       return PipelineBlendMode::None;
        case Shader::BlendMode::AlphaBlend: return PipelineBlendMode::AlphaBlend;
        case Shader::BlendMode::Additive:   return PipelineBlendMode::Additive;
        default:                            return PipelineBlendMode::None;
        }
    }

    static inline PipelineCullMode ToPipelineCullMode(Shader::CullMode mode) {
        switch (mode) {
        case Shader::CullMode::None:         return PipelineCullMode::None;
        case Shader::CullMode::Front:        return PipelineCullMode::Front;
        case Shader::CullMode::Back:         return PipelineCullMode::Back;
        case Shader::CullMode::FrontAndBack: return PipelineCullMode::FrontAndBack;
        default:                             return PipelineCullMode::Back;
        }
    }

    static inline PipelineFillMode ToPipelineFillMode(Shader::FillMode mode) {
        switch (mode) {
        case Shader::FillMode::Solid:     return PipelineFillMode::Solid;
        case Shader::FillMode::Wireframe: return PipelineFillMode::Wireframe;
        default:                          return PipelineFillMode::Solid;
        }
    }

    static inline VkPrimitiveTopology ToVkPrimitiveTopology(Shader::PrimitiveTopology topo) {
        switch (topo) {
        case Shader::PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case Shader::PrimitiveTopology::LineList:     return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case Shader::PrimitiveTopology::PointList:    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case Shader::PrimitiveTopology::PatchList:    return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
        }
        return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }

    VulkanShader::VulkanShader(const ShaderDescription& desc)
        : m_Description(desc)
    {
        for (const auto& u : m_Description.globalUniforms) m_GlobalUniformMap[u.name] = &u;
        for (const auto& u : m_Description.objectUniforms) m_ObjectUniformMap[u.name] = &u;

        m_Stages = CreateShaderStages(VulkanContext::Context.device->device);
        Q_DEBUG("Vulkan shader initialized successfully");

        const uint32_t swapchainImageCount = static_cast<uint32_t>(VulkanContext::Context.swapchain->images.size());

        std::vector<VkDescriptorSetLayoutBinding> globalBindings;
        if (!m_Description.globalUniforms.empty()) {
            const auto& first = m_Description.globalUniforms[0];
            VkDescriptorSetLayoutBinding b{};
            b.binding = first.binding;
            b.descriptorCount = 1;
            b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ShaderStageFlags stages = 0;
            for (const auto& u : m_Description.globalUniforms) stages |= u.stages;
            b.stageFlags = ShaderStageFlagsToVk(stages);
            b.pImmutableSamplers = nullptr;
            globalBindings.push_back(b);
        }

        VkDescriptorSetLayoutCreateInfo globalLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        globalLayoutInfo.bindingCount = static_cast<uint32_t>(globalBindings.size());
        globalLayoutInfo.pBindings = globalBindings.empty() ? nullptr : globalBindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(
            VulkanContext::Context.device->device,
            &globalLayoutInfo,
            VulkanContext::Context.allocator->GetCallbacks(),
            &globalDescriptorSetLayout));

        std::vector<VkDescriptorPoolSize> globalPoolSizes;
        if (!globalBindings.empty()) {
            VkDescriptorPoolSize s{};
            s.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            s.descriptorCount = swapchainImageCount;
            globalPoolSizes.push_back(s);
        }

        VkDescriptorPoolCreateInfo globalPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        globalPoolInfo.poolSizeCount = static_cast<uint32_t>(globalPoolSizes.size());
        globalPoolInfo.pPoolSizes = globalPoolSizes.empty() ? nullptr : globalPoolSizes.data();
        globalPoolInfo.maxSets = swapchainImageCount;

        VK_CHECK(vkCreateDescriptorPool(
            VulkanContext::Context.device->device,
            &globalPoolInfo,
            VulkanContext::Context.allocator->GetCallbacks(),
            &globalDescriptorPool));

        std::vector<VkDescriptorSetLayoutBinding> objectBindings;

        if (!m_Description.objectUniforms.empty()) {
            const auto& first = m_Description.objectUniforms[0];
            VkDescriptorSetLayoutBinding b{};
            b.binding = first.binding;
            b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            b.descriptorCount = 1;
            ShaderStageFlags stages = 0;
            for (const auto& u : m_Description.objectUniforms) stages |= u.stages;
            b.stageFlags = ShaderStageFlagsToVk(stages);
            b.pImmutableSamplers = nullptr;
            objectBindings.push_back(b);
        }

        for (const auto& sampler : m_Description.samplers) {
            VkDescriptorSetLayoutBinding b{};
            b.binding = sampler.binding;
            b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            b.descriptorCount = 1;
            b.stageFlags = ShaderStageFlagsToVk(sampler.stages);
            b.pImmutableSamplers = nullptr;
            objectBindings.push_back(b);
        }

        m_HasObjectSet = !objectBindings.empty();

        VkDescriptorSetLayoutCreateInfo objectLayoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        objectLayoutInfo.bindingCount = static_cast<uint32_t>(objectBindings.size());
        objectLayoutInfo.pBindings = objectBindings.empty() ? nullptr : objectBindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(
            VulkanContext::Context.device->device,
            &objectLayoutInfo,
            VulkanContext::Context.allocator->GetCallbacks(),
            &objectDescriptorSetLayout));

        if (m_HasObjectSet) {
            std::vector<VkDescriptorPoolSize> objectPoolSizes;
            const uint32_t samplerCount = static_cast<uint32_t>(m_Description.samplers.size());
            const bool hasObjectUBOBinding = !m_Description.objectUniforms.empty();

            if (hasObjectUBOBinding) {
                VkDescriptorPoolSize ubo{};
                ubo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                ubo.descriptorCount = MAX_OBJECTS * swapchainImageCount;
                objectPoolSizes.push_back(ubo);
            }
            if (samplerCount > 0) {
                VkDescriptorPoolSize samp{};
                samp.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                samp.descriptorCount = samplerCount * MAX_OBJECTS * swapchainImageCount;
                objectPoolSizes.push_back(samp);
            }

            VkDescriptorPoolCreateInfo objectPoolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
            objectPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            objectPoolInfo.poolSizeCount = static_cast<uint32_t>(objectPoolSizes.size());
            objectPoolInfo.pPoolSizes = objectPoolSizes.data();
            objectPoolInfo.maxSets = MAX_OBJECTS * swapchainImageCount;

            VK_CHECK(vkCreateDescriptorPool(
                VulkanContext::Context.device->device,
                &objectPoolInfo,
                VulkanContext::Context.allocator->GetCallbacks(),
                &objectDescriptorPool));
        }
        else {
            objectDescriptorPool = VK_NULL_HANDLE;
        }

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(VulkanContext::Context.height);
        viewport.width = static_cast<float>(VulkanContext::Context.width);
        viewport.height = -static_cast<float>(VulkanContext::Context.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = { VulkanContext::Context.width, VulkanContext::Context.height };

        const auto* vertexModule = !desc.modules.empty() ? &desc.modules[0] : nullptr;
        for (const auto& m : desc.modules) {
            if (m.stage == Shader::ShaderStageType::Vertex) { vertexModule = &m; break; }
        }

        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        uint32_t vertexStride = 0;
        if (vertexModule) {
            for (const auto& input : vertexModule->inputs) {
                VkVertexInputAttributeDescription attr{};
                attr.location = input.location;
                attr.binding = 0;
                attr.format = ShaderIOTypeToVkFormat(input.type);
                attr.offset = vertexStride;
                attributeDescriptions.push_back(attr);
                vertexStride += ShaderIOTypeToSize(input.type);
            }
        }

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
        descriptorSetLayouts.push_back(globalDescriptorSetLayout);
        descriptorSetLayouts.push_back(objectDescriptorSetLayout);

        std::vector<VkPipelineShaderStageCreateInfo> stageInfos(m_Stages.size());
        for (uint32_t i = 0; i < m_Stages.size(); ++i) stageInfos[i] = m_Stages[i].stage_info;

        std::vector<VkPushConstantRange> pushConstants;
        for (const auto& pc : desc.pushConstants) {
            VkPushConstantRange r{};
            r.stageFlags = ShaderStageFlagsToVk(pc.stages);
            r.offset = pc.offset;
            r.size = pc.size;
            pushConstants.push_back(r);
        }

        VulkanPipelineDesc pipelineDesc;
        pipelineDesc.renderPass = m_Description.framebuffer
            ? static_cast<VulkanFramebuffer*>(m_Description.framebuffer)->GetRenderPass()->renderpass
            : VulkanContext::Context.mainRenderPass->renderpass;

        pipelineDesc.attributes = attributeDescriptions;
        pipelineDesc.descriptorSetLayouts = descriptorSetLayouts;
        pipelineDesc.stages = stageInfos;
        pipelineDesc.pushConstants = pushConstants;
        pipelineDesc.stride = vertexStride;

        pipelineDesc.viewport = viewport;
        pipelineDesc.scissor = scissor;

        pipelineDesc.topology = ToVkPrimitiveTopology(m_Description.topology);
        pipelineDesc.patchControlPoints = m_Description.patchControlPoints;

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
        for (const auto& u : m_Description.globalUniforms)
            globalUBOSize = std::max(globalUBOSize, u.offset + u.size);

        m_HasGlobalUBO = (globalUBOSize > 0);
        if (m_HasGlobalUBO) {
            m_GlobalUniformData.resize(globalUBOSize);

            const uint32_t device_local_bits =
                VulkanContext::Context.device->supportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;

            globalUniformBuffer = std::make_unique<VulkanBuffer>(
                VulkanContext::Context.device->device,
                VulkanContext::Context.device->physicalDevice,
                globalUBOSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | device_local_bits,
                true);

            std::vector<VkDescriptorSetLayout> globalLayouts(swapchainImageCount, globalDescriptorSetLayout);

            VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            allocInfo.descriptorPool = globalDescriptorPool;
            allocInfo.descriptorSetCount = swapchainImageCount;
            allocInfo.pSetLayouts = globalLayouts.data();

            globalDescriptorSets.resize(swapchainImageCount);
            VK_CHECK(vkAllocateDescriptorSets(VulkanContext::Context.device->device, &allocInfo, globalDescriptorSets.data()));
        }
        else {
            m_GlobalUniformData.clear();
            globalDescriptorSets.clear();
        }

        size_t objectUBOSize = 0;
        for (const auto& u : m_Description.objectUniforms)
            objectUBOSize = std::max(objectUBOSize, u.offset + u.size);

        m_HasObjectUBO = (objectUBOSize > 0);
        const size_t minAlignment = VulkanContext::Context.device->physicalDeviceInfo.limits.minUniformBufferOffsetAlignment;
        m_ObjectStride = AlignUBOOffset(objectUBOSize, minAlignment);

        if (m_HasObjectUBO) {
            m_ObjectUniformData.resize(objectUBOSize);
            objectUniformBuffer = std::make_unique<VulkanBuffer>(
                VulkanContext::Context.device->device,
                VulkanContext::Context.device->physicalDevice,
                m_ObjectStride * MAX_OBJECTS,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                true);
        }
        else {
            m_ObjectUniformData.clear();
        }

        TextureSpecification spec;
        spec.width = 2; spec.height = 2;
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
            bluePixels[i * 4 + 0] = 0;   // R
            bluePixels[i * 4 + 1] = 0;   // G
            bluePixels[i * 4 + 2] = 255; // B
            bluePixels[i * 4 + 3] = 255; // A
        }
        defaultBlueTexture = new VulkanTexture2D(spec);
        defaultBlueTexture->LoadFromData({ bluePixels.data(), bluePixels.size() });

        Q_ASSERT(defaultBlueTexture != nullptr, "defaultBlueTexture nullptr");
        Q_ASSERT(defaultBlueTexture->sampler != VK_NULL_HANDLE, "defaultBlueTexture->sampler nullptr");
        Q_ASSERT(defaultBlueTexture->image != nullptr, "defaultBlueTexture->image nullptr");
        Q_ASSERT(defaultBlueTexture->image->view != VK_NULL_HANDLE, "defaultBlueTexture->image->view nullptr");
    }

    VulkanShader::~VulkanShader()
    {
        delete defaultBlueTexture;
        defaultBlueTexture = nullptr;

        objectUniformBuffer.reset();
        globalUniformBuffer.reset();
        pipeline.reset();

        VkDevice device = VulkanContext::Context.device->device;

        if (objectDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, objectDescriptorPool, VulkanContext::Context.allocator->GetCallbacks());
            objectDescriptorPool = VK_NULL_HANDLE;
        }
        if (objectDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, objectDescriptorSetLayout, VulkanContext::Context.allocator->GetCallbacks());
            objectDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (globalDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, globalDescriptorPool, VulkanContext::Context.allocator->GetCallbacks());
            globalDescriptorPool = VK_NULL_HANDLE;
        }
        if (globalDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, globalDescriptorSetLayout, VulkanContext::Context.allocator->GetCallbacks());
            globalDescriptorSetLayout = VK_NULL_HANDLE;
        }

        for (auto& s : m_Stages) {
            if (s.handle != VK_NULL_HANDLE) {
                vkDestroyShaderModule(device, s.handle, VulkanContext::Context.allocator->GetCallbacks());
                s.handle = VK_NULL_HANDLE;
            }
        }
        m_Stages.clear();

        objectStates.clear();
        globalDescriptorSets.clear();

        objectUniformBufferIndex = 0;
        c_offset = 0;
        m_NextObjectId = 0;
        m_FreeIds.clear();
    }

    std::vector<VulkanShaderStage> VulkanShader::CreateShaderStages(VkDevice device)
    {
        std::vector<VulkanShaderStage> stages;
        stages.reserve(m_Description.modules.size());

        for (const auto& moduleInfo : m_Description.modules) {
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
        if (!m_HasGlobalUBO) return true;

        const uint32_t imageIndex = VulkanContext::Context.imageIndex;
        if (imageIndex >= globalDescriptorSets.size()) return false;
        const VkDescriptorSet globalDescriptor = globalDescriptorSets[imageIndex];

        if (!globalUniformBuffer || globalUniformBuffer->handle == VK_NULL_HANDLE) {
            Q_ERROR("globalUniformBuffer handle is VK_NULL_HANDLE!");
            return false;
        }

        if (!m_GlobalUniformData.empty())
            globalUniformBuffer->LoadData(0, m_GlobalUniformData.size(), 0, m_GlobalUniformData.data());

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = globalUniformBuffer->handle;
        bufferInfo.offset = 0;
        bufferInfo.range = m_GlobalUniformData.size();

        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = globalDescriptor;
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(VulkanContext::Context.device->device, 1, &write, 0, nullptr);

        vkCmdBindDescriptorSets(
            VulkanContext::Context.frameCommandBuffers.back()->handle,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->layout,
            0,
            1, &globalDescriptor,
            0, nullptr);

        return true;
    }

    bool VulkanShader::UpdateObject(Material* material)
    {
        if (!material) return false;
        if (material->m_ID == INVALID_ID) {
            Q_ERROR("Material has INVALID_ID (resources not acquired or released).");
            return false;
        }

        const uint32_t id = material->m_ID;
        if (id >= MAX_OBJECTS) {
            Q_ERROR("Object ID exceeds maximum limit");
            return false;
        }
        if (id >= objectStates.size()) {
            Q_ERROR("Object ID is out of range");
            return false;
        }

        if (!m_HasObjectSet) return true;

        ObjectShaderObjectState* objectState = &objectStates[id];
        const uint32_t imageIndex = VulkanContext::Context.imageIndex;

        if (objectState->descriptorSets.empty() || imageIndex >= objectState->descriptorSets.size()) {
            Q_ERROR("Descriptor set index out of range");
            return false;
        }
        const VkDescriptorSet objectDescriptorSet = objectState->descriptorSets[imageIndex];
        if (objectDescriptorSet == VK_NULL_HANDLE) {
            Q_ERROR("objectDescriptorSet is VK_NULL_HANDLE!");
            return false;
        }

        if (objectState->lastMaterialGenerationPerImage.size() != objectState->descriptorSets.size()) {
            objectState->lastMaterialGenerationPerImage.assign(objectState->descriptorSets.size(), INVALID_ID);
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites;

        VkDescriptorBufferInfo bufferInfo{};
        if (m_HasObjectUBO) {
            if (!objectUniformBuffer || objectUniformBuffer->handle == VK_NULL_HANDLE) {
                Q_ERROR("objectUniformBuffer handle is VK_NULL_HANDLE!");
                return false;
            }

            const size_t structSize = m_ObjectUniformData.size();
            const size_t bufferOffset = static_cast<size_t>(id) * m_ObjectStride;

            if (structSize > 0)
                objectUniformBuffer->LoadData(bufferOffset, structSize, 0, m_ObjectUniformData.data());

            bufferInfo.buffer = objectUniformBuffer->handle;
            bufferInfo.offset = bufferOffset;
            bufferInfo.range = structSize;

            VkWriteDescriptorSet uboWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            uboWrite.dstSet = objectDescriptorSet;
            uboWrite.dstBinding = 0;
            uboWrite.dstArrayElement = 0;
            uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboWrite.descriptorCount = 1;
            uboWrite.pBufferInfo = &bufferInfo;

            descriptorWrites.push_back(uboWrite);
        }

        bool needSamplerUpdate = (objectState->lastMaterialGenerationPerImage[imageIndex] != material->m_Generation);
        std::vector<VkDescriptorImageInfo> imageInfos;

        if (!m_Description.samplers.empty()) {
            if (objectState->boundSamplers.size() != m_Description.samplers.size())
                objectState->boundSamplers.assign(m_Description.samplers.size(), nullptr);

            if (needSamplerUpdate) {
                imageInfos.resize(m_Description.samplers.size());

                for (size_t i = 0; i < m_Description.samplers.size(); ++i) {
                    const auto& samplerDesc = m_Description.samplers[i];
                    VulkanTexture2D* tex = nullptr;

                    auto it = m_ObjectTextures.find(samplerDesc.name);
                    if (it != m_ObjectTextures.end() && it->second)
                        tex = it->second;

                    if (!tex) tex = defaultBlueTexture;

                    if (!tex || !tex->image || tex->image->view == VK_NULL_HANDLE || tex->sampler == VK_NULL_HANDLE) {
                        Q_ERROR("Texture/sampler invalide pour '%s' (tex=%p, view=%p, sampler=%p)",
                            samplerDesc.name.c_str(),
                            tex,
                            tex && tex->image ? (void*)tex->image->view : nullptr,
                            tex ? (void*)tex->sampler : nullptr);
                        continue;
                    }

                    objectState->boundSamplers[i] = tex;

                    imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    imageInfos[i].imageView = tex->image->view;
                    imageInfos[i].sampler = tex->sampler;

                    VkWriteDescriptorSet sampWrite{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                    sampWrite.dstSet = objectDescriptorSet;
                    sampWrite.dstBinding = samplerDesc.binding;
                    sampWrite.dstArrayElement = 0;
                    sampWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    sampWrite.descriptorCount = 1;
                    sampWrite.pImageInfo = &imageInfos[i];

                    descriptorWrites.push_back(sampWrite);
                }
            }
        }

        if (!descriptorWrites.empty()) {
            vkUpdateDescriptorSets(
                VulkanContext::Context.device->device,
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0, nullptr);
        }

        auto* cmd = VulkanContext::Context.frameCommandBuffers.back().get();
        if (!cmd || cmd->handle == VK_NULL_HANDLE || pipeline->layout == VK_NULL_HANDLE) {
            Q_ERROR("Invalid command buffer or pipeline layout");
            return false;
        }

        vkCmdBindDescriptorSets(
            cmd->handle,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline->layout,
            1,
            1, &objectDescriptorSet,
            0, nullptr);

        if (needSamplerUpdate)
            objectState->lastMaterialGenerationPerImage[imageIndex] = material->m_Generation;

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
            data);
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
        if (!material) return false;

        uint32_t id;
        if (!m_FreeIds.empty()) {
            id = m_FreeIds.back();
            m_FreeIds.pop_back();
        }
        else {
            if (m_NextObjectId >= MAX_OBJECTS) {
                Q_ERROR("MAX_OBJECTS atteint (%u)", MAX_OBJECTS);
                return false;
            }
            id = m_NextObjectId++;
        }

        ObjectShaderObjectState object_state;

        if (m_HasObjectSet) {
            const uint32_t swapchainImageCount = (uint32_t)VulkanContext::Context.swapchain->images.size();
            std::vector<VkDescriptorSetLayout> layouts(swapchainImageCount, objectDescriptorSetLayout);
            object_state.descriptorSets.resize(swapchainImageCount);

            VkDescriptorSetAllocateInfo alloc_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            alloc_info.descriptorPool = objectDescriptorPool;
            alloc_info.descriptorSetCount = swapchainImageCount;
            alloc_info.pSetLayouts = layouts.data();

            VkResult res = vkAllocateDescriptorSets(
                VulkanContext::Context.device->device,
                &alloc_info,
                object_state.descriptorSets.data());

            if (res != VK_SUCCESS) {
                Q_ERROR("Error allocating descriptor sets in shader");
                m_FreeIds.push_back(id);
                return false;
            }

            const uint32_t bindingCount = (m_HasObjectUBO ? 1u : 0u) + static_cast<uint32_t>(m_Description.samplers.size());
            object_state.descriptorStates.resize(bindingCount);
            for (uint32_t i = 0; i < bindingCount; ++i) {
                for (uint32_t j = 0; j < std::min<uint32_t>(3, swapchainImageCount); ++j) {
                    object_state.descriptorStates[i].generations[j] = INVALID_ID;
                    object_state.descriptorStates[i].ids[j] = INVALID_ID;
                }
            }

            object_state.lastMaterialGenerationPerImage.assign(swapchainImageCount, INVALID_ID);
        }
        else {
            object_state.descriptorSets.clear();
            object_state.descriptorStates.clear();
            object_state.lastMaterialGenerationPerImage.clear();
        }

        object_state.boundSamplers.resize(m_Description.samplers.size(), nullptr);

        if (objectStates.size() <= id) objectStates.resize(id + 1);
        objectStates[id] = std::move(object_state);

        material->m_ID;
        return true;
    }

    void VulkanShader::ReleaseResources(Material* material)
    {
        if (!material) return;

        const uint32_t id = material->m_ID;
        if (id == INVALID_ID || id >= objectStates.size())
            return;

        ObjectShaderObjectState* objectState = &objectStates[id];

        if (m_HasObjectSet && !objectState->descriptorSets.empty()) {
            VkResult result = vkFreeDescriptorSets(
                VulkanContext::Context.device->device,
                objectDescriptorPool,
                static_cast<uint32_t>(objectState->descriptorSets.size()),
                objectState->descriptorSets.data());
            if (result != VK_SUCCESS) {
                Q_ERROR("Error freeing object shader descriptor sets");
            }
        }

        objectStates[id] = {};
        m_FreeIds.push_back(id);

        material->m_ID = INVALID_ID;
    }

    void VulkanShader::SetUniform(const std::string& name, void* data, size_t size)
    {
        const ShaderUniformDesc* d = nullptr;
        auto it = m_GlobalUniformMap.find(name);
        if (it != m_GlobalUniformMap.end()) {
            d = it->second;
            if (size != d->size) {
                Q_ERROR("Uniform '%s' size mismatch (got %zu, expected %zu)", name.c_str(), size, d->size);
                return;
            }
            if (d->offset + size > m_GlobalUniformData.size()) {
                Q_ERROR("Uniform '%s' write out of bounds (global UBO)", name.c_str());
                return;
            }
            std::memcpy(m_GlobalUniformData.data() + d->offset, data, size);
            return;
        }
        it = m_ObjectUniformMap.find(name);
        if (it != m_ObjectUniformMap.end()) {
            d = it->second;
            if (size != d->size) {
                Q_ERROR("Uniform '%s' size mismatch (got %zu, expected %zu)", name.c_str(), size, d->size);
                return;
            }
            if (d->offset + size > m_ObjectUniformData.size()) {
                Q_ERROR("Uniform '%s' write out of bounds (object UBO)", name.c_str());
                return;
            }
            std::memcpy(m_ObjectUniformData.data() + d->offset, data, size);
            return;
        }
        Q_ERROR("Uniform '%s' not found", name.c_str());
    }

    void VulkanShader::SetTexture(const std::string& name, Texture* texture, SamplerType /*type*/)
    {
        auto it = std::find_if(
            m_Description.samplers.begin(), m_Description.samplers.end(),
            [&](const ShaderSamplerDesc& d) { return d.name == name; });

        if (it == m_Description.samplers.end()) {
            Q_ERROR("Sampler '%s' not found in shader description!", name.c_str());
            return;
        }

        VulkanTexture2D* vkTex = nullptr;
        if (texture) {
            vkTex = dynamic_cast<VulkanTexture2D*>(texture);
            if (!vkTex) {
                Q_WARNING("Texture for sampler '%s' is not a VulkanTexture2D. Falling back to default.", name.c_str());
            }
        }
        m_ObjectTextures[name] = vkTex ? vkTex : defaultBlueTexture;
    }
}
