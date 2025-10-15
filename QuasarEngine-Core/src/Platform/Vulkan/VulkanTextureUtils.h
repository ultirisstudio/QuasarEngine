#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <QuasarEngine/Resources/TextureTypes.h>

namespace QuasarEngine
{
    namespace Utils
    {
        struct Map {
            VkFormat    format{ VK_FORMAT_UNDEFINED };
            uint32_t    channels{ 0 };
        };

        static Map MapTextureFormat(TextureFormat fmt) {
            switch (fmt) {
            case TextureFormat::RED:
            case TextureFormat::RED8:
                return { VK_FORMAT_R8_UNORM, 1 };

            case TextureFormat::RGB:
            case TextureFormat::RGB8:
                return { VK_FORMAT_R8G8B8_UNORM, 3 };

            case TextureFormat::RGBA:
            case TextureFormat::RGBA8:
                return { VK_FORMAT_R8G8B8A8_UNORM, 4 };

            case TextureFormat::SRGB:
            case TextureFormat::SRGB8:
                return { VK_FORMAT_R8G8B8_SRGB, 3 };

            case TextureFormat::SRGBA:
            case TextureFormat::SRGB8A8:
                return { VK_FORMAT_R8G8B8A8_SRGB, 4 };
            }
            return { VK_FORMAT_UNDEFINED, 0 };
        }

        static VkFormat TextureFormatToVulkan(TextureFormat fmt)
        {
            switch (fmt) {
            case TextureFormat::RED:
            case TextureFormat::RED8:     return VK_FORMAT_R8_UNORM;

            case TextureFormat::RGB:
            case TextureFormat::RGB8:     return VK_FORMAT_R8G8B8_UNORM;
            case TextureFormat::SRGB:
            case TextureFormat::SRGB8:    return VK_FORMAT_R8G8B8_SRGB;

            case TextureFormat::RGBA:
            case TextureFormat::RGBA8:    return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::SRGBA:
            case TextureFormat::SRGB8A8:  return VK_FORMAT_R8G8B8A8_SRGB;
            }
            return VK_FORMAT_UNDEFINED;
        }

        static uint32_t DesiredChannels(TextureFormat fmt)
        {
            switch (fmt) {
            case TextureFormat::RED:
            case TextureFormat::RED8:     return 1;
            case TextureFormat::RGB:
            case TextureFormat::RGB8:
            case TextureFormat::SRGB:
            case TextureFormat::SRGB8:    return 3;
            case TextureFormat::RGBA:
            case TextureFormat::RGBA8:
            case TextureFormat::SRGBA:
            case TextureFormat::SRGB8A8:  return 4;
            }
            return 0;
        }

        static uint32_t BytesPerPixel(VkFormat fmt)
        {
            switch (fmt) {
            case VK_FORMAT_R8_UNORM:          return 1;
            case VK_FORMAT_R8G8B8_UNORM:      return 3;
            case VK_FORMAT_R8G8B8_SRGB:       return 3;
            case VK_FORMAT_R8G8B8A8_UNORM:    return 4;
            case VK_FORMAT_R8G8B8A8_SRGB:     return 4;
            default:                          return 4;
            }
        }

        static VkFilter ToVkFilter(TextureFilter filter)
        {
            switch (filter) {
            case TextureFilter::NEAREST:               return VK_FILTER_NEAREST;
            case TextureFilter::LINEAR:                return VK_FILTER_LINEAR;
            case TextureFilter::NEAREST_MIPMAP_NEAREST:
            case TextureFilter::NEAREST_MIPMAP_LINEAR: return VK_FILTER_NEAREST;
            case TextureFilter::LINEAR_MIPMAP_NEAREST:
            case TextureFilter::LINEAR_MIPMAP_LINEAR:  return VK_FILTER_LINEAR;
            }
            return VK_FILTER_LINEAR;
        }

        static VkSamplerMipmapMode ToVkMipmapMode(TextureFilter filter)
        {
            switch (filter) {
            case TextureFilter::NEAREST_MIPMAP_NEAREST:
            case TextureFilter::LINEAR_MIPMAP_NEAREST: return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case TextureFilter::NEAREST_MIPMAP_LINEAR:
            case TextureFilter::LINEAR_MIPMAP_LINEAR:  return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            case TextureFilter::NEAREST:
            case TextureFilter::LINEAR:
            default:                                   return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
        }

        static VkSamplerAddressMode ToVkAddress(TextureWrap wrap) {
            switch (wrap) {
            case TextureWrap::REPEAT:          return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case TextureWrap::MIRRORED_REPEAT: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case TextureWrap::CLAMP_TO_EDGE:   return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureWrap::CLAMP_TO_BORDER: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            }
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }

        static uint32_t CalcMipmapLevels(uint32_t w, uint32_t h, bool mipmap)
        {
            if (!mipmap) return 1;
            uint32_t levels = 1;
            uint32_t m = (w > h) ? w : h;
            while (m > 1) { m >>= 1; ++levels; }
            return levels;
        }

        static VkImageUsageFlags DefaultTextureUsage(bool mipmap)
        {
            VkImageUsageFlags flags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (mipmap) flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            return flags;
        }

        static VkSampleCountFlagBits SamplesToVk(uint32_t samples)
        {
            switch (samples) {
            case 1:  return VK_SAMPLE_COUNT_1_BIT;
            case 2:  return VK_SAMPLE_COUNT_2_BIT;
            case 4:  return VK_SAMPLE_COUNT_4_BIT;
            case 8:  return VK_SAMPLE_COUNT_8_BIT;
            case 16: return VK_SAMPLE_COUNT_16_BIT;
            case 32: return VK_SAMPLE_COUNT_32_BIT;
            case 64: return VK_SAMPLE_COUNT_64_BIT;
            default: return VK_SAMPLE_COUNT_1_BIT;
            }
        }

        static VkImageType  TextureImageType(bool multisampled) { return VK_IMAGE_TYPE_2D; }
        static VkImageViewType TextureViewType(bool multisampled) { return VK_IMAGE_VIEW_TYPE_2D; }
    }
}