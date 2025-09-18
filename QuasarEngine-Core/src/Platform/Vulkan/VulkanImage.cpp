#include "qepch.h"
#include "VulkanImage.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"

namespace QuasarEngine
{
	namespace Utils
	{
		static uint32_t BytesPerPixel(VkFormat format) {
			switch (format) {
			case VK_FORMAT_R8_UNORM: return 1;
			case VK_FORMAT_R8G8B8_UNORM: return 3;
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_R8G8B8A8_SRGB: return 4;
			case VK_FORMAT_R8G8B8_SRGB: return 3;
			default: return 4;
			}
		}
	}

	VulkanImage::VulkanImage(VkImageType type, VkImageViewType viewType, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, bool createView, VkImageAspectFlags viewAspectFlags, uint32_t layerCount, VkImageCreateFlags createFlags, uint32_t mipLevels)
		: width(width), height(height), format(format)
	{
		VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageInfo.imageType = type;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = layerCount;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = createFlags;

		VK_CHECK(vkCreateImage(VulkanContext::Context.device->device, &imageInfo, VulkanContext::Context.allocator->GetCallbacks(), &handle));

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(VulkanContext::Context.device->device, handle, &memRequirements);

		int32_t memoryTypeIndex = FindMemoryIndex(memRequirements.memoryTypeBits, memoryFlags);
		if (memoryTypeIndex == -1)
		{
			Q_ERROR("Failed to find suitable memory type!");
			return;
		}

		VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = memoryTypeIndex;

		VK_CHECK(vkAllocateMemory(VulkanContext::Context.device->device, &allocInfo, VulkanContext::Context.allocator->GetCallbacks(), &memory));

		VK_CHECK(vkBindImageMemory(VulkanContext::Context.device->device, handle, memory, 0));

		if (createView)
		{
			view = nullptr;
			CreateImageView(format, viewAspectFlags, viewType, layerCount);
		}
	}

	void VulkanImage::ImageTransitionLayout(VkCommandBuffer cmd, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t levelCount, uint32_t baseArrayLayer, uint32_t layerCount)
	{
		assert(layerCount > 0 && "layerCount doit être >= 1");
		assert(levelCount > 0 && "levelCount doit être >= 1");

		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = handle;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = baseMipLevel;
		barrier.subresourceRange.levelCount = levelCount;
		barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
		barrier.subresourceRange.layerCount = layerCount;

		VkPipelineStageFlags source_stage;
		VkPipelineStageFlags dest_stage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			Q_WARNING("Unsupported layout transition requested.");
			return;
		}

		vkCmdPipelineBarrier(cmd, source_stage, dest_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void VulkanImage::CopyFromBuffer(VkCommandBuffer cmd, VkBuffer buffer, uint32_t layerCount)
	{
		std::vector<VkBufferImageCopy> regions(layerCount);

		uint32_t layerSize = width * height * Utils::BytesPerPixel(format);

		for (uint32_t i = 0; i < layerCount; ++i)
		{
			VkBufferImageCopy& region = regions[i];
			region.bufferOffset = static_cast<VkDeviceSize>(i * layerSize);
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = i;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { width, height, 1 };
		}

		vkCmdCopyBufferToImage(cmd, buffer, handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()), regions.data());
	}

	void VulkanImage::GenerateMipmaps(VkCommandBuffer cmd, VkFormat format, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layerCount)
	{
		int32_t mipWidth = static_cast<int32_t>(width);
		int32_t mipHeight = static_cast<int32_t>(height);

		for (uint32_t i = 1; i < mipLevels; ++i)
		{
			ImageTransitionLayout(cmd, format,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				i - 1, 1,
				0, layerCount);

			for (uint32_t layer = 0; layer < layerCount; ++layer)
			{
				VkImageBlit blit{};
				blit.srcOffsets[0] = { 0, 0, 0 };
				blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
				blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.srcSubresource.mipLevel = i - 1;
				blit.srcSubresource.baseArrayLayer = layer;
				blit.srcSubresource.layerCount = 1;

				blit.dstOffsets[0] = { 0, 0, 0 };
				blit.dstOffsets[1] = { std::max(mipWidth / 2, 1), std::max(mipHeight / 2, 1), 1 };
				blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				blit.dstSubresource.mipLevel = i;
				blit.dstSubresource.baseArrayLayer = layer;
				blit.dstSubresource.layerCount = 1;

				vkCmdBlitImage(cmd,
					handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &blit,
					VK_FILTER_LINEAR);
			}

			ImageTransitionLayout(cmd, format,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				i - 1, 1,
				0, layerCount);

			mipWidth = std::max(mipWidth / 2, 1);
			mipHeight = std::max(mipHeight / 2, 1);
		}

		ImageTransitionLayout(cmd, format,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			mipLevels - 1, 1,
			0, layerCount);
	}


	int32_t VulkanImage::FindMemoryIndex(uint32_t typeFilter, uint32_t propertyFlags)
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(VulkanContext::Context.device->physicalDevice, &memory_properties);

		for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
		{
			if ((typeFilter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
			{
				return i;
			}
		}

		Q_WARNING("Unable to find suitable memory type !");
		return -1;
	}

	void VulkanImage::CreateImageView(VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType, uint32_t layerCount)
	{
		VkImageViewCreateInfo viewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewInfo.image = handle;
		viewInfo.viewType = viewType;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = aspectFlags;

		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = layerCount;

		VK_CHECK(vkCreateImageView(VulkanContext::Context.device->device, &viewInfo, VulkanContext::Context.allocator->GetCallbacks(), &view));
	}

	VulkanImage::~VulkanImage()
	{
		if (view)
		{
			vkDestroyImageView(VulkanContext::Context.device->device, view, VulkanContext::Context.allocator->GetCallbacks());
			view = nullptr;
		}

		if (memory)
		{
			vkFreeMemory(VulkanContext::Context.device->device, memory, VulkanContext::Context.allocator->GetCallbacks());
			memory = nullptr;
		}

		if (handle)
		{
			vkDestroyImage(VulkanContext::Context.device->device, handle, VulkanContext::Context.allocator->GetCallbacks());
			handle = nullptr;
		}
	}
}