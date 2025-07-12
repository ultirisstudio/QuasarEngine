#pragma once

#include "VulkanTypes.h"

#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
	class VulkanImage;

	class VulkanTexture2D : public Texture2D
	{
	public:
		VulkanTexture2D(const TextureSpecification& specification);
		~VulkanTexture2D() override;

		void LoadFromPath(const std::string& path) override;
		void LoadFromMemory(unsigned char* image_data, size_t size) override;
		void LoadFromData(unsigned char* image_data, size_t size) override;

		void* GetHandle() const override { return descriptor; }

		void Bind(int index) const override;
		void Unbind() const override;

		std::unique_ptr<VulkanImage> image;
		VkSampler sampler;
		uint32_t generation;

		VkDescriptorSet descriptor;
	};
}
