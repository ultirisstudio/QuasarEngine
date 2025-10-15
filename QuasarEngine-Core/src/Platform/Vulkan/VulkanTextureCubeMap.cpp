#include "qepch.h"

#include "VulkanTextureCubeMap.h"

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanCommandBuffer.h"
#include "VulkanImage.h"

#include <QuasarEngine/File/FileUtils.h>
#include <QuasarEngine/Core/Logger.h>

#include <backends/imgui_impl_vulkan.h>
#include <stb_image.h>

#include "VulkanTextureUtils.h"

#include <algorithm>
#include <cmath>

namespace QuasarEngine
{
    VulkanTextureCubeMap::VulkanTextureCubeMap(const TextureSpecification& specification)
        : TextureCubeMap(specification) {
        m_HasFace.fill(false);
    }

    VulkanTextureCubeMap::~VulkanTextureCubeMap() {
        vkDeviceWaitIdle(VulkanContext::Context.device->device);

        image.reset();

        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(VulkanContext::Context.device->device, sampler, VulkanContext::Context.allocator->GetCallbacks());
            sampler = VK_NULL_HANDLE;
        }

        if (descriptor) { ImGui_ImplVulkan_RemoveTexture(descriptor); descriptor = VK_NULL_HANDLE; }
    }

    bool VulkanTextureCubeMap::LoadFromPath(const std::string& path) {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("VulkanTextureCubeMap: failed to read file: " + path);
            return false;
        }
        return LoadFromMemory(ByteView{ bytes.data(), bytes.size() });
    }

    bool VulkanTextureCubeMap::LoadFromMemory(ByteView data) {
        if (data.empty()) {
            Q_ERROR("VulkanTextureCubeMap: empty memory buffer");
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
        const int desired = static_cast<int>(Utils::DesiredChannels(m_Specification.internal_format));
        unsigned char* decoded = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.data),
            static_cast<int>(data.size),
            &w, &h, &actual, desired
        );
        if (!decoded) {
            Q_ERROR(std::string("VulkanTextureCubeMap: stb_image decode failed: ") + stbi_failure_reason());
            return false;
        }

        m_Specification.width = static_cast<uint32_t>(w);
        m_Specification.height = static_cast<uint32_t>(h);
        m_Specification.channels = static_cast<uint32_t>(desired ? desired : actual);

        const std::size_t faceSize = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * m_Specification.channels;
        std::vector<std::uint8_t> all(6 * faceSize);
        for (int i = 0; i < 6; ++i) {
            std::memcpy(all.data() + i * faceSize, decoded, faceSize);
        }

        stbi_image_free(decoded);
        return LoadFromData(ByteView{ all.data(), all.size() });
    }

    bool VulkanTextureCubeMap::LoadFromData(ByteView data) {
        if (data.empty()) {
            Q_ERROR("VulkanTextureCubeMap: no pixel data");
            return false;
        }
        if (m_Specification.width == 0 || m_Specification.height == 0) {
            Q_ERROR("VulkanTextureCubeMap: width/height must be set before LoadFromData()");
            return false;
        }

        VkFormat vkFormat = Utils::TextureFormatToVulkan(m_Specification.internal_format);
        if (vkFormat == VK_FORMAT_UNDEFINED) {
            Q_ERROR("VulkanTextureCubeMap: unsupported internal format");
            return false;
        }

        const uint32_t bpp = Utils::BytesPerPixel(vkFormat);
        const std::size_t faceSize =
            static_cast<std::size_t>(m_Specification.width) *
            static_cast<std::size_t>(m_Specification.height) *
            static_cast<std::size_t>(bpp);

        if (data.size == faceSize * 6) {
            return UploadAllFaces(data);
        }
        else if (data.size == faceSize) {
            std::vector<std::uint8_t> all(6 * faceSize);
            for (int i = 0; i < 6; ++i) {
                std::memcpy(all.data() + i * faceSize, data.data, faceSize);
            }
            return UploadAllFaces(ByteView{ all.data(), all.size() });
        }
        else {
            Q_ERROR("VulkanTextureCubeMap: data size must be faceSize or 6 * faceSize");
            return false;
        }
    }

    bool VulkanTextureCubeMap::LoadFaceFromPath(Face face, const std::string& path) {
        auto bytes = FileUtils::ReadFileBinary(path);
        if (bytes.empty()) {
            Q_ERROR("VulkanTextureCubeMap: failed to read face file: " + path);
            return false;
        }
        return LoadFaceFromMemory(face, ByteView{ bytes.data(), bytes.size() });
    }

    bool VulkanTextureCubeMap::LoadFaceFromMemory(Face face, ByteView data) {
        if (data.empty()) {
            Q_ERROR("VulkanTextureCubeMap: empty memory buffer (face)");
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
        const int desired = static_cast<int>(Utils::DesiredChannels(m_Specification.internal_format));
        unsigned char* decoded = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(data.data),
            static_cast<int>(data.size),
            &w, &h, &actual, desired
        );
        if (!decoded) {
            Q_ERROR(std::string("VulkanTextureCubeMap: stb_image face decode failed: ") + stbi_failure_reason());
            return false;
        }

        const bool ok = LoadFaceFromData(face,
            ByteView{ decoded, static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * static_cast<std::size_t>(desired ? desired : actual) },
            static_cast<uint32_t>(w), static_cast<uint32_t>(h), static_cast<uint32_t>(desired ? desired : actual));
        stbi_image_free(decoded);
        return ok;
    }

    bool VulkanTextureCubeMap::LoadFaceFromData(Face face, ByteView data, uint32_t w, uint32_t h, uint32_t channels) {
        if (data.empty() || w == 0 || h == 0 || channels == 0) {
            Q_ERROR("VulkanTextureCubeMap: invalid face data");
            return false;
        }

        const uint32_t expectedChannels = Utils::DesiredChannels(m_Specification.internal_format);
        if (expectedChannels != 0 && channels != expectedChannels) {
            Q_ERROR("VulkanTextureCubeMap: channels mismatch with internal format");
            return false;
        }

        if (!IsComplete(m_HasFace) && m_FaceW == 0 && m_FaceH == 0) {
            m_FaceW = w; m_FaceH = h; m_FaceChannels = channels;
            m_Specification.width = w; m_Specification.height = h; m_Specification.channels = channels;
        }
        else {
            if (m_FaceW != w || m_FaceH != h || m_FaceChannels != channels) {
                Q_ERROR("VulkanTextureCubeMap: face dimensions/channels mismatch with staged faces");
                return false;
            }
        }

        const std::size_t faceSize = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * static_cast<std::size_t>(channels);
        m_StagedFaces[FaceIndex(face)].assign(data.data, data.data + faceSize);
        m_HasFace[FaceIndex(face)] = true;

        return TryUploadIfComplete();
    }

    bool VulkanTextureCubeMap::TryUploadIfComplete() {
        if (!IsComplete(m_HasFace)) return true;

        const uint32_t w = m_FaceW, h = m_FaceH, c = m_FaceChannels;
        const std::size_t faceSize = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * static_cast<std::size_t>(c);

        std::vector<std::uint8_t> all(6 * faceSize);
        for (int i = 0; i < 6; ++i) {
            std::memcpy(all.data() + i * faceSize, m_StagedFaces[i].data(), faceSize);
        }

        m_HasFace.fill(false);
        for (auto& v : m_StagedFaces) v.clear();

        return UploadAllFaces(ByteView{ all.data(), all.size() });
    }

    bool VulkanTextureCubeMap::UploadAllFaces(ByteView allFacesPixels) {
        const VkDevice device = VulkanContext::Context.device->device;
        const VkQueue  queue = VulkanContext::Context.device->graphicsQueue;
        const VkCommandPool pool = VulkanContext::Context.device->graphicsCommandPool;

        VkFormat vkFormat = Utils::TextureFormatToVulkan(m_Specification.internal_format);
        if (vkFormat == VK_FORMAT_UNDEFINED) {
            Q_ERROR("VulkanTextureCubeMap: unsupported internal format");
            return false;
        }

        const uint32_t mipLevels =
            Utils::CalcMipmapLevels(m_Specification.width, m_Specification.height, m_Specification.mipmap);

        const std::size_t expectedTotal =
            static_cast<std::size_t>(m_Specification.width) *
            static_cast<std::size_t>(m_Specification.height) *
            static_cast<std::size_t>(Utils::BytesPerPixel(vkFormat)) * 6;

        if (allFacesPixels.size != expectedTotal) {
            Q_ERROR("VulkanTextureCubeMap: allFacesPixels has unexpected size");
            return false;
        }

        const VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        const VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        auto staging = std::make_unique<VulkanBuffer>(
            device,
            VulkanContext::Context.device->physicalDevice,
            expectedTotal,
            usage,
            memFlags,
            true
        );
        staging->LoadData(0, expectedTotal, 0, allFacesPixels.data);

        image = std::make_unique<VulkanImage>(
            VK_IMAGE_TYPE_2D,
            VK_IMAGE_VIEW_TYPE_CUBE,
            m_Specification.width,
            m_Specification.height,
            vkFormat,
            VK_IMAGE_TILING_OPTIMAL,
            Utils::DefaultTextureUsage(m_Specification.mipmap),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            true,
            VK_IMAGE_ASPECT_COLOR_BIT,
            6,
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            mipLevels
        );

        auto cmd = std::make_unique<VulkanCommandBuffer>(device, pool);
        cmd->AllocateAndBeginSingleUse();

        image->ImageTransitionLayout(cmd->handle, vkFormat,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            0, mipLevels,
            0, 6);

        image->CopyFromBuffer(cmd->handle, staging->handle, 6);

        if (m_Specification.mipmap && mipLevels > 1) {
            image->GenerateMipmaps(cmd->handle, vkFormat,
                m_Specification.width, m_Specification.height,
                mipLevels, 6);
        }
        else {
            image->ImageTransitionLayout(cmd->handle, vkFormat,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                0, mipLevels,
                0, 6);
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
            Q_ERROR("VulkanTextureCubeMap: error creating sampler: " + VulkanResultString(vr));
            return false;
        }

        descriptor = ImGui_ImplVulkan_AddTexture(sampler, image->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        generation++;
        m_Loaded = true;
        return true;
    }

    void VulkanTextureCubeMap::Bind(int /*index*/) const {
        
    }

    void VulkanTextureCubeMap::Unbind() const {
        
    }
}