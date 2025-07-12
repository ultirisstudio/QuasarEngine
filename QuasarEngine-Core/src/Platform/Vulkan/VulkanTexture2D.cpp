#include "qepch.h"

#include "VulkanTexture2D.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImage.h"

#include <QuasarEngine/File/FileUtils.h>

#include <backends/imgui_impl_vulkan.h>

#include <stb_image.h>

#define INVALID_ID 4294967295U

namespace QuasarEngine
{
	namespace Utils
	{
		static VkImageType TextureImageType(bool multisampled)
		{
			return VK_IMAGE_TYPE_2D;
		}

		static VkImageViewType TextureViewType(bool multisampled)
		{
			return VK_IMAGE_VIEW_TYPE_2D;
		}

		static VkFormat TextureFormatToVulkan(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGB:   return VK_FORMAT_R8G8B8_UNORM;
			case TextureFormat::RGBA:  return VK_FORMAT_R8G8B8A8_UNORM;
			case TextureFormat::SRGB:  return VK_FORMAT_R8G8B8_SRGB;
			case TextureFormat::SRGBA: return VK_FORMAT_R8G8B8A8_SRGB;
			case TextureFormat::RED:   return VK_FORMAT_R8_UNORM;
			}
			return VK_FORMAT_UNDEFINED;
		}

		static VkSamplerAddressMode TextureWrapToVulkan(TextureWrap wrap)
		{
			switch (wrap)
			{
			case TextureWrap::REPEAT:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case TextureWrap::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case TextureWrap::CLAMP_TO_EDGE:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case TextureWrap::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			}
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		}

		static VkFilter TextureFilterToVulkan(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::NEAREST:               return VK_FILTER_NEAREST;
			case TextureFilter::LINEAR:                return VK_FILTER_LINEAR;
			case TextureFilter::NEAREST_MIPMAP_NEAREST:
			case TextureFilter::LINEAR_MIPMAP_NEAREST:
			case TextureFilter::NEAREST_MIPMAP_LINEAR:
			case TextureFilter::LINEAR_MIPMAP_LINEAR:
				return (filter == TextureFilter::NEAREST_MIPMAP_NEAREST || filter == TextureFilter::NEAREST_MIPMAP_LINEAR)
					? VK_FILTER_NEAREST
					: VK_FILTER_LINEAR;
			}
			return VK_FILTER_LINEAR;
		}

		static VkSamplerMipmapMode TextureMipmapModeToVulkan(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::NEAREST_MIPMAP_NEAREST:
			case TextureFilter::LINEAR_MIPMAP_NEAREST:
				return VK_SAMPLER_MIPMAP_MODE_NEAREST;
			case TextureFilter::NEAREST_MIPMAP_LINEAR:
			case TextureFilter::LINEAR_MIPMAP_LINEAR:
				return VK_SAMPLER_MIPMAP_MODE_LINEAR;
			default:
				return VK_SAMPLER_MIPMAP_MODE_LINEAR;
			}
		}

		static int DesiredChannelFromTextureFormat(TextureFormat format)
		{
			switch (format)
			{
			case TextureFormat::RGB: return 3;
			case TextureFormat::RGBA: return 4;
			case TextureFormat::RED: return 1;
			}
			return 0;
		}

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

	VulkanTexture2D::VulkanTexture2D(const TextureSpecification& specification) : Texture2D(specification), sampler(VK_NULL_HANDLE), generation(INVALID_ID)
	{
		
	}

	VulkanTexture2D::~VulkanTexture2D()
	{
		vkDeviceWaitIdle(VulkanContext::Context.device->device);

		image.reset();

		if (sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(VulkanContext::Context.device->device, sampler, VulkanContext::Context.allocator->GetCallbacks());
		}

		//if (descriptor)
		//{
			//ImGui_ImplVulkan_RemoveTexture(descriptor);
		//}
	}

	void VulkanTexture2D::LoadFromPath(const std::string& path)
	{
		std::vector<unsigned char> buffer = FileUtils::readBinaryFile(path);

		LoadFromMemory(buffer.data(), buffer.size());
	}

	void VulkanTexture2D::LoadFromMemory(unsigned char* image_data, size_t size)
	{
		if (m_Specification.compressed)
		{
			if (m_Specification.flip)
			{
				stbi_set_flip_vertically_on_load(true);
			}
			else
			{
				stbi_set_flip_vertically_on_load(false);
			}

			if (m_Specification.alpha)
			{
				m_Specification.format = TextureFormat::RGBA;
				m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGBA : TextureFormat::RGBA;
			}
			else
			{
				m_Specification.format = TextureFormat::RGBA; // TODO:  TextureFormat::RGB
				m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGBA : TextureFormat::RGBA; // TODO: TextureFormat::SRGB : TextureFormat::RGB
			}

			int width, height, nbChannels;
			unsigned char* data = stbi_load_from_memory(image_data, size, &width, &height, &nbChannels, 4); // TODO: Utils::DesiredChannelFromTextureFormat(m_Specification.internal_format)

			if (!data)
			{
				Q_ERROR(std::string("Failed to decode image from memory: ") + stbi_failure_reason());
				return;
			}

			m_Specification.width = width;
			m_Specification.height = height;
			m_Specification.channels = nbChannels;

			LoadFromData(data, width * height * Utils::BytesPerPixel(Utils::TextureFormatToVulkan(m_Specification.internal_format)));

			stbi_image_free(data);
		}
		else
		{
			LoadFromData(image_data, m_Specification.width * m_Specification.height * 4); // TODO: Utils::BytesPerPixel(Utils::TextureFormatToVulkan(m_Specification.internal_format))
		}
	}

	void VulkanTexture2D::LoadFromData(unsigned char* image_data, size_t size)
	{
		uint32_t mipLevels = m_Specification.mipmap ? static_cast<uint32_t>(std::floor(std::log2(std::max(m_Specification.width, m_Specification.height)))) + 1 : 1;

		VkFormat image_format = Utils::TextureFormatToVulkan(m_Specification.internal_format);

		VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		VkMemoryPropertyFlags memory_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		std::unique_ptr<VulkanBuffer> staging = std::make_unique<VulkanBuffer>(
			VulkanContext::Context.device->device,
			VulkanContext::Context.device->physicalDevice,
			size,
			usage,
			memory_prop_flags,
			true
		);

		staging->LoadData(0, size, 0, image_data);

		image = std::make_unique<VulkanImage>(
			VK_IMAGE_TYPE_2D,
			VK_IMAGE_VIEW_TYPE_2D,
			m_Specification.width,
			m_Specification.height,
			image_format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			true,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1,
			0,
			mipLevels);

		VkDevice device = VulkanContext::Context.device->device;
		VkCommandPool pool = VulkanContext::Context.device->graphicsCommandPool;
		VkQueue queue = VulkanContext::Context.device->graphicsQueue;

		std::unique_ptr<VulkanCommandBuffer> temp_buffer = std::make_unique<VulkanCommandBuffer>(device, pool);

		temp_buffer->AllocateAndBeginSingleUse();

		image->ImageTransitionLayout(temp_buffer->handle, image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, mipLevels, 0, 1);
		image->CopyFromBuffer(temp_buffer->handle, staging->handle, 1);

		if (m_Specification.mipmap)
		{
			image->GenerateMipmaps(temp_buffer->handle, image_format, m_Specification.width, m_Specification.height, mipLevels, 1);
		}
		else
		{
			image->ImageTransitionLayout(temp_buffer->handle, image_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, mipLevels, 0, 1);
		}

		temp_buffer->EndSingleUse(queue);

		temp_buffer.reset();
		staging.reset();

		VkSamplerCreateInfo sampler_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		sampler_info.magFilter = Utils::TextureFilterToVulkan(m_Specification.mag_filter_param);
		sampler_info.minFilter = Utils::TextureFilterToVulkan(m_Specification.min_filter_param);
		sampler_info.addressModeU = Utils::TextureWrapToVulkan(m_Specification.wrap_r);
		sampler_info.addressModeV = Utils::TextureWrapToVulkan(m_Specification.wrap_s);
		sampler_info.addressModeW = Utils::TextureWrapToVulkan(m_Specification.wrap_t);
		sampler_info.anisotropyEnable = VK_TRUE;
		sampler_info.maxAnisotropy = VulkanContext::Context.device->physicalDeviceInfo.limits.maxSamplerAnisotropy;
		sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		sampler_info.unnormalizedCoordinates = VK_FALSE;
		sampler_info.compareEnable = VK_FALSE;
		sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
		sampler_info.mipmapMode = Utils::TextureMipmapModeToVulkan(TextureFilter::LINEAR_MIPMAP_LINEAR);
		sampler_info.mipLodBias = 0.0f;
		sampler_info.minLod = 0.0f;
		sampler_info.maxLod = m_Specification.mipmap ? static_cast<float>(mipLevels) : 0.0f;

		VkResult result = vkCreateSampler(device, &sampler_info, VulkanContext::Context.allocator->GetCallbacks(), &sampler);
		if (!VulkanResultIsSuccess(result))
		{
			std::string str = "Error creating texture sampler: " + VulkanResultString(result);
			Q_ERROR(str);
			return;
		}

		descriptor = ImGui_ImplVulkan_AddTexture(sampler, image->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		generation++;
	}

	void VulkanTexture2D::Bind(int index) const
	{
		
	}

	void VulkanTexture2D::Unbind() const
	{
		
	}
}