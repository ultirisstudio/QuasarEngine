#include "qepch.h"

#include "VulkanTexture2D.h"
#include "VulkanTextureUtils.h"

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImage.h"

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

#include <backends/imgui_impl_vulkan.h>
#include <stb_image.h>
#include <cmath>
#include <algorithm>

namespace QuasarEngine
{
    VulkanTexture2D::VulkanTexture2D(const TextureSpecification& specification)
        : Texture2D(specification) {
    }

    VulkanTexture2D::~VulkanTexture2D() {
        vkDeviceWaitIdle(VulkanContext::Context.device->device);

        image.reset();

        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(VulkanContext::Context.device->device, sampler, VulkanContext::Context.allocator->GetCallbacks());
            sampler = VK_NULL_HANDLE;
        }

        if (descriptor) { ImGui_ImplVulkan_RemoveTexture(descriptor); descriptor = VK_NULL_HANDLE; }
    }

    bool VulkanTexture2D::LoadFromPath(const std::string& path) {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("VulkanTexture2D: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool VulkanTexture2D::LoadFromMemory(ByteView data) {
        if (data.empty()) {
            Q_ERROR("VulkanTexture2D: empty memory buffer");
            return false;
        }

        if (m_Specification.flip)  stbi_set_flip_vertically_on_load(true);
        else                       stbi_set_flip_vertically_on_load(false);

        if (m_Specification.alpha) {
            m_Specification.format = TextureFormat::RGBA;
            m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB8A8 : TextureFormat::RGBA8;
        }
        else {
            m_Specification.format = TextureFormat::RGB;
            m_Specification.internal_format = m_Specification.gamma ? TextureFormat::SRGB8 : TextureFormat::RGB8;
        }

        int w = 0, h = 0, actual = 0;
        const int desired = static_cast<int>(Utils::MapTextureFormat(m_Specification.internal_format).channels);
        unsigned char* decoded = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.data),
            static_cast<int>(data.size),
            &w, &h, &actual, desired
        );

        if (!decoded) {
            Q_ERROR(std::string("stb_image decode failed: ") + stbi_failure_reason());
            return false;
        }

        m_Specification.width = static_cast<uint32_t>(w);
        m_Specification.height = static_cast<uint32_t>(h);
        m_Specification.channels = static_cast<uint32_t>(desired ? desired : actual);

        const bool ok = LoadFromData(ByteView{ decoded, static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * m_Specification.channels });
        stbi_image_free(decoded);
        return ok;
    }

    bool VulkanTexture2D::LoadFromData(ByteView pixels) {
        if (pixels.empty()) {
            Q_ERROR("VulkanTexture2D: no pixel data");
            return false;
        }
        if (m_Specification.width == 0 || m_Specification.height == 0) {
            Q_ERROR("VulkanTexture2D: width/height must be set before LoadFromData()");
            return false;
        }
        return Upload(pixels);
    }

    glm::vec4 VulkanTexture2D::Sample(const glm::vec2& uv) const
    {
        return glm::vec4();
    }

    bool VulkanTexture2D::Upload(ByteView pixels) {
        const VkDevice device = VulkanContext::Context.device->device;
        const VkQueue  queue = VulkanContext::Context.device->graphicsQueue;
        const VkCommandPool pool = VulkanContext::Context.device->graphicsCommandPool;

        const auto mapInt = Utils::MapTextureFormat(m_Specification.internal_format);
        if (mapInt.format == VK_FORMAT_UNDEFINED || mapInt.channels == 0) {
            Q_ERROR("VulkanTexture2D: unsupported internal format");
            return false;
        }

        const uint32_t bpp = Utils::BytesPerPixel(mapInt.format);
        const std::size_t expected = static_cast<std::size_t>(m_Specification.width) *
            static_cast<std::size_t>(m_Specification.height) *
            static_cast<std::size_t>(bpp);
        if (pixels.size != expected) {
            Q_WARNING("VulkanTexture2D: pixel data size mismatch (got " + std::to_string(pixels.size) +
                ", expected " + std::to_string(expected) + ")");
            if (pixels.size < expected) return false;
        }

        const uint32_t mipLevels = Utils::CalcMipmapLevels(m_Specification.width, m_Specification.height, m_Specification.mipmap);

        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        auto staging = std::make_unique<VulkanBuffer>(
            device,
            VulkanContext::Context.device->physicalDevice,
            expected,
            usage,
            memFlags,
            true
        );
        staging->LoadData(0, expected, 0, pixels.data);

        image = std::make_unique<VulkanImage>(
            VK_IMAGE_TYPE_2D,
            VK_IMAGE_VIEW_TYPE_2D,
            m_Specification.width,
            m_Specification.height,
            mapInt.format,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_COLOR_BIT,
            1,
            0,
            mipLevels
        );

        auto cmd = std::make_unique<VulkanCommandBuffer>(device, pool);
        cmd->AllocateAndBeginSingleUse();

        image->ImageTransitionLayout(cmd->handle, mapInt.format,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, mipLevels, 0, 1);

        image->CopyFromBuffer(cmd->handle, staging->handle, 1);

        if (m_Specification.mipmap && mipLevels > 1) {
            image->GenerateMipmaps(cmd->handle, mapInt.format,
                m_Specification.width, m_Specification.height,
                mipLevels, 1);
        }
        else {
            image->ImageTransitionLayout(cmd->handle, mapInt.format,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                0, mipLevels, 0, 1);
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
            Q_ERROR("Error creating texture sampler: " + VulkanResultString(vr));
            return false;
        }

        descriptor = ImGui_ImplVulkan_AddTexture(sampler, image->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        generation++;
        m_Loaded = true;
        return true;
    }

    void VulkanTexture2D::Bind(int index) const {
        
    }

    void VulkanTexture2D::Unbind() const {
        
    }
}
