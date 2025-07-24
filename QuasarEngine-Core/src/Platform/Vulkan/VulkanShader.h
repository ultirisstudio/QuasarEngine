#pragma once

#include "VulkanTypes.h"

#include <QuasarEngine/Shader/Shader.h>

namespace QuasarEngine
{
	struct DescriptorState
	{
		uint32_t generations[3];
		uint32_t ids[3];
	};

	struct ObjectShaderObjectState
	{
		std::vector<VkDescriptorSet> descriptorSets;
		std::vector<DescriptorState> descriptorStates;
	};

	struct VulkanShaderStage
	{
		VkShaderModule handle = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo stage_info{};
		Shader::ShaderStageType stage_type{};
		std::string path;
	};

	class VulkanPipeline;
	class VulkanBuffer;

	class VulkanTexture2D;

	class Material;

	class VulkanShader : public Shader
	{
	public:
		VulkanShader(const ShaderDescription& desc);
		~VulkanShader() override;

		void Use() override;
		void Unuse() override;
		void Reset() override;

		bool AcquireResources(Material* material) override;
		void ReleaseResources(Material* material) override;

		std::vector<VulkanShaderStage> CreateShaderStages(VkDevice device);
		VkShaderModule CreateVkShaderModule(VkDevice device, const std::vector<uint32_t>& code);

		bool UpdateGlobalState() override;
		bool UpdateObject(Material* material) override;

		void PushConstant(const void* data, size_t size, VkShaderStageFlags stageFlags, uint32_t offset);

		std::vector<VulkanShaderStage> m_Stages;

		std::unique_ptr<VulkanPipeline> pipeline;

		VkDescriptorPool globalDescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;

		std::unique_ptr<VulkanBuffer> globalUniformBuffer;

		std::vector<VkDescriptorSet> globalDescriptorSets;

		VkDescriptorPool objectDescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout objectDescriptorSetLayout = VK_NULL_HANDLE;

		std::unique_ptr<VulkanBuffer> objectUniformBuffer;

		uint32_t objectUniformBufferIndex;

		std::vector<ObjectShaderObjectState> objectStates;

		uint32_t c_offset;

		ShaderDescription m_Description;

		std::unordered_map<std::string, VulkanTexture2D*> m_ObjectTextures;
		std::unordered_map<std::string, Shader::SamplerType> m_ObjectTextureTypes;

		std::unordered_map<std::string, const ShaderUniformDesc*> m_GlobalUniformMap;
		std::unordered_map<std::string, const ShaderUniformDesc*> m_ObjectUniformMap;

		std::vector<uint8_t> m_GlobalUniformData;
		std::vector<uint8_t> m_ObjectUniformData;

		VulkanTexture2D* defaultBlueTexture;

		void SetUniform(const std::string& name, void* data, size_t size) override;

		void SetTexture(const std::string& name, Texture* texture, SamplerType type) override;
	};
}