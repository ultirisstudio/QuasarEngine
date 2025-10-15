#include "qepch.h"

#include "VulkanTextureArray.h"
#include "VulkanTextureUtils.h"

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImage.h"

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

#include <backends/imgui_impl_vulkan.h>

#include <algorithm>
#include <cmath>

namespace QuasarEngine
{
    VulkanTextureArray::VulkanTextureArray(const TextureSpecification& specification)
        : TextureArray(specification) {
    }

    VulkanTextureArray::~VulkanTextureArray() {
        vkDeviceWaitIdle(VulkanContext::Context.device->device);

        image.reset();

        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(VulkanContext::Context.device->device, sampler, VulkanContext::Context.allocator->GetCallbacks());
            sampler = VK_NULL_HANDLE;
        }

        if (descriptor) { ImGui_ImplVulkan_RemoveTexture(descriptor); descriptor = VK_NULL_HANDLE; }
    }

    bool VulkanTextureArray::LoadFromPath(const std::string& path) {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("VulkanTextureArray: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool VulkanTextureArray::LoadFromMemory(ByteView data) {
        return LoadFromData(data);
    }

    bool VulkanTextureArray::LoadFromData(ByteView pixels) {
        if (pixels.empty()) {
            Q_ERROR("VulkanTextureArray: no pixel data");
            return false;
        }
        if (m_Specification.width == 0 || m_Specification.height == 0) {
            Q_ERROR("VulkanTextureArray: width/height must be set before LoadFromData()");
            return false;
        }

        VkFormat vkFormat = Utils::TextureFormatToVulkan(m_Specification.internal_format);
        if (vkFormat == VK_FORMAT_UNDEFINED) {
            Q_ERROR("VulkanTextureArray: unsupported internal format");
            return false;
        }
        const uint32_t bpp = Utils::BytesPerPixel(vkFormat);

        const std::size_t layerSize =
            static_cast<std::size_t>(m_Specification.width) *
            static_cast<std::size_t>(m_Specification.height) *
            static_cast<std::size_t>(bpp);

        if (layerSize == 0) {
            Q_ERROR("VulkanTextureArray: invalid layer size");
            return false;
        }
        if (pixels.size % layerSize != 0) {
            Q_ERROR("VulkanTextureArray: data size doesn't match width*height*bpp*layers");
            return false;
        }

        const uint32_t layers = static_cast<uint32_t>(pixels.size / layerSize);
        m_Specification.channels = Utils::DesiredChannels(m_Specification.internal_format);

        return Upload(pixels, layers);
    }

    bool VulkanTextureArray::Upload(ByteView pixels, uint32_t layers) {
        const VkDevice device = VulkanContext::Context.device->device;
        const VkQueue  queue = VulkanContext::Context.device->graphicsQueue;
        const VkCommandPool pool = VulkanContext::Context.device->graphicsCommandPool;

        VkFormat vkFormat = Utils::TextureFormatToVulkan(m_Specification.internal_format);
        if (vkFormat == VK_FORMAT_UNDEFINED) {
            Q_ERROR("VulkanTextureArray: unsupported internal format");
            return false;
        }

        const uint32_t mipLevels =
            Utils::CalcMipmapLevels(m_Specification.width, m_Specification.height, m_Specification.mipmap);

        const std::size_t totalBytes =
            static_cast<std::size_t>(m_Specification.width) *
            static_cast<std::size_t>(m_Specification.height) *
            static_cast<std::size_t>(Utils::BytesPerPixel(vkFormat)) *
            static_cast<std::size_t>(layers);

        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        auto staging = std::make_unique<VulkanBuffer>(
            device,
            VulkanContext::Context.device->physicalDevice,
            totalBytes,
            usage,
            memFlags,
            true
        );
        staging->LoadData(0, totalBytes, 0, pixels.data);

        image = std::make_unique<VulkanImage>(
            VK_IMAGE_TYPE_2D,
            VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            m_Specification.width,
            m_Specification.height,
            vkFormat,
            VK_IMAGE_TILING_OPTIMAL,
            Utils::DefaultTextureUsage(m_Specification.mipmap),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_COLOR_BIT,
            layers,
            0,
            mipLevels
        );

        auto cmd = std::make_unique<VulkanCommandBuffer>(device, pool);
        cmd->AllocateAndBeginSingleUse();

        image->ImageTransitionLayout(cmd->handle, vkFormat,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, mipLevels,
            0, layers);

        image->CopyFromBuffer(cmd->handle, staging->handle, layers);

        if (m_Specification.mipmap && mipLevels > 1) {
            image->GenerateMipmaps(cmd->handle, vkFormat,
                m_Specification.width, m_Specification.height,
                mipLevels, layers);
        }
        else {
            image->ImageTransitionLayout(cmd->handle, vkFormat,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                0, mipLevels,
                0, layers);
        }

        cmd->EndSingleUse(queue);
        cmd.reset();
        staging.reset();

        VkSamplerCreateInfo sInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        sInfo.magFilter = Utils::ToVkFilter(m_Specification.mag_filter_param);
        sInfo.minFilter = Utils::ToVkFilter(m_Specification.min_filter_param);
        sInfo.mipmapMode = Utils::ToVkMipmapMode(m_Specification.min_filter_param);
        sInfo.addressModeU = Utils::ToVkAddress(m_Specification.wrap_s);
        sInfo.addressModeV = Utils::ToVkAddress(m_Specification.wrap_t);
        sInfo.addressModeW = Utils::ToVkAddress(m_Specification.wrap_r);
        sInfo.mipLodBias = 0.0f;
        sInfo.minLod = 0.0f;
        sInfo.maxLod = m_Specification.mipmap ? static_cast<float>(mipLevels) : 0.0f;
        sInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sInfo.unnormalizedCoordinates = VK_FALSE;
        sInfo.compareEnable = VK_FALSE;
        sInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        sInfo.anisotropyEnable = VK_TRUE;
        sInfo.maxAnisotropy = VulkanContext::Context.device->physicalDeviceInfo.limits.maxSamplerAnisotropy;

        VkResult vr = vkCreateSampler(device, &sInfo, VulkanContext::Context.allocator->GetCallbacks(), &sampler);
        if (!VulkanResultIsSuccess(vr)) {
            Q_ERROR("VulkanTextureArray: error creating sampler: " + VulkanResultString(vr));
            return false;
        }

        descriptor = ImGui_ImplVulkan_AddTexture(sampler, image->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        generation++;
        m_Loaded = true;
        return true;
    }

    void VulkanTextureArray::Bind(int index) const {
        
    }

    void VulkanTextureArray::Unbind() const {
        
    }
}