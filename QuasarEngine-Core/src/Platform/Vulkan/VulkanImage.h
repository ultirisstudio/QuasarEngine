#pragma once

#include "VulkanTypes.h"

namespace QuasarEngine
{
	class VulkanBuffer;

	class VulkanImage
	{
	public:
		VulkanImage(VkImageType type, VkImageViewType viewType, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, bool createView, VkImageAspectFlags viewAspectFlags, uint32_t layerCount = 1, VkImageCreateFlags createFlags = 0, uint32_t mipLevels = 1);
		~VulkanImage();

		void CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, uint32_t layerCount = 1, uint32_t mipLevels = 1);

		void ImageTransitionLayout(VkCommandBuffer cmd, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel = 0, uint32_t levelCount = 1, uint32_t baseArrayLayer = 0, uint32_t layerCount = 1);
		void CopyFromBuffer(VkCommandBuffer cmd, VkBuffer buffer, uint32_t layerCount = 1);

		void GenerateMipmaps(VkCommandBuffer cmd, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels = 1, uint32_t layerCount = 1);

		VkFormat GetFormat() const { return format; }

		int32_t FindMemoryIndex(uint32_t typeFilter, uint32_t propertyFlags);

		VkImage handle;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		uint32_t width;
		uint32_t height;
	};
}