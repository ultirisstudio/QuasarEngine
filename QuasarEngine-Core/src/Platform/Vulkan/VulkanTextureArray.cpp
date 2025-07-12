#include "qepch.h"

#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanTextureArray.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"

#include <stb_image.h>
#include <backends/imgui_impl_vulkan.h>

namespace QuasarEngine
{
    namespace Utils
    {
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
    }

	VulkanTextureArray::VulkanTextureArray(const TextureSpecification& specification)
		: TextureArray(specification), sampler(VK_NULL_HANDLE), descriptor(VK_NULL_HANDLE), generation(0)
	{
		
	}

	VulkanTextureArray::~VulkanTextureArray()
	{
		vkDeviceWaitIdle(VulkanContext::Context.device->device);

		image.reset();

		if (sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(VulkanContext::Context.device->device, sampler, VulkanContext::Context.allocator->GetCallbacks());
		}
	}

	void VulkanTextureArray::LoadFromFiles(const std::vector<std::string>& paths)
	{
        std::vector<unsigned char*> pixels;
        int texWidth = 0, texHeight = 0, channels = 0;

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

        arrayLayers = static_cast<uint32_t>(paths.size());
        for (size_t i = 0; i < paths.size(); ++i)
        {
            int width, height, nbChannels;
            //stbi_set_flip_vertically_on_load(false);
            unsigned char* data = stbi_load(paths[i].c_str(), &width, &height, &nbChannels, STBI_rgb_alpha); // TODO: Utils::DesiredChannelFromTextureFormat(m_Specification.internal_format)
            if (!data)
            {
                for (auto p : pixels) stbi_image_free(p);
                throw std::runtime_error("Erreur lors du chargement d'une texture array : " + paths[i]);
            }
            if (i == 0)
            {
                texWidth = width; texHeight = height; channels = nbChannels;
            }
            else if (width != texWidth || height != texHeight)
            {
                stbi_image_free(data);
                for (auto p : pixels) stbi_image_free(p);
                throw std::runtime_error("Toutes les textures d'un array doivent avoir la même taille !");
            }
            pixels.push_back(data);
        }

        m_Specification.width = texWidth;
        m_Specification.height = texHeight;
        m_Specification.channels = channels;

        uint32_t mipLevels = m_Specification.mipmap ? static_cast<uint32_t>(std::floor(std::log2(std::max(m_Specification.width, m_Specification.height)))) + 1 : 1;

        VkFormat image_format = Utils::TextureFormatToVulkan(m_Specification.internal_format);

        VkDevice device = VulkanContext::Context.device->device;
        VkPhysicalDevice physicalDevice = VulkanContext::Context.device->physicalDevice;
        VkCommandPool pool = VulkanContext::Context.device->graphicsCommandPool;
        VkQueue queue = VulkanContext::Context.device->graphicsQueue;

        VkDeviceSize layerSize = texWidth * texHeight * channels;
        VkDeviceSize totalSize = layerSize * arrayLayers;

        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags memory_prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        std::unique_ptr<VulkanBuffer> staging = std::make_unique<VulkanBuffer>(
            device,
            physicalDevice,
            totalSize,
            usage,
            memory_prop_flags,
            true
        );

        std::vector<unsigned char> temp(totalSize);
        for (uint32_t layer = 0; layer < arrayLayers; ++layer)
            memcpy(temp.data() + layer * layerSize, pixels[layer], layerSize);

        staging->LoadData(0, totalSize, 0, temp.data());

        image = std::make_unique<VulkanImage>(
            VK_IMAGE_TYPE_2D,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            m_Specification.width,
            m_Specification.height,
            image_format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_COLOR_BIT,
            arrayLayers,
            0,
            mipLevels);

        std::unique_ptr<VulkanCommandBuffer> temp_buffer = std::make_unique<VulkanCommandBuffer>(device, pool);

        temp_buffer->AllocateAndBeginSingleUse();

        image->ImageTransitionLayout(temp_buffer->handle, image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, mipLevels, 0, arrayLayers);
        image->CopyFromBuffer(temp_buffer->handle, staging->handle, arrayLayers);

        if (m_Specification.mipmap)
        {
            image->GenerateMipmaps(temp_buffer->handle, image_format, m_Specification.width, m_Specification.height, mipLevels, arrayLayers);
        }
        else
        {
            image->ImageTransitionLayout(temp_buffer->handle, image_format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, mipLevels, 0, arrayLayers);
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

        for (auto p : pixels) stbi_image_free(p);
	}
}
