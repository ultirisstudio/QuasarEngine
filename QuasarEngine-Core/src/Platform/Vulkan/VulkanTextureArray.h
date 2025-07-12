#pragma once

#include "VulkanTypes.h"

#include <QuasarEngine/Resources/TextureArray.h>

namespace QuasarEngine
{
	class VulkanImage;

	class VulkanTextureArray : public TextureArray
	{
	public:
		VulkanTextureArray(const TextureSpecification& specification);
		~VulkanTextureArray() override;

		void Bind(int index = 0) const override {};
		void Unbind() const override {};

		void* GetHandle() const override { return nullptr; };

		void LoadFromFiles(const std::vector<std::string>& paths) override;

		void LoadFromPath(const std::string& path) override {};
		void LoadFromMemory(unsigned char* image_data, size_t size) override {};
		void LoadFromData(unsigned char* image_data, size_t size) override {};

	private:
		std::unique_ptr<VulkanImage> image;
		VkSampler sampler;
		VkDescriptorSet descriptor;
		uint32_t arrayLayers;
		uint32_t generation;
	};
}