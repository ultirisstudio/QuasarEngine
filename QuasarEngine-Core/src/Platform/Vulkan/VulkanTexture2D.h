#pragma once

#include "VulkanTypes.h"

#include <QuasarEngine/Resources/Texture2D.h>
#include <vulkan/vulkan.h>
#include <memory>

namespace QuasarEngine {

    class VulkanImage;

    class VulkanTexture2D : public Texture2D {
    public:
        explicit VulkanTexture2D(const TextureSpecification& specification);
        ~VulkanTexture2D() override;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(reinterpret_cast<uintptr_t>(descriptor)); }
        bool IsLoaded() const noexcept override { return m_Loaded; }

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView pixels) override;

        void Bind(int index = 0) const override;
        void Unbind() const override;

    public:
        std::unique_ptr<VulkanImage> image{};
        VkSampler        sampler{ VK_NULL_HANDLE };
        uint32_t         generation{ 0 };
        VkDescriptorSet  descriptor{ VK_NULL_HANDLE };

    private:
        struct Map {
            VkFormat    format{ VK_FORMAT_UNDEFINED };
            uint32_t    channels{ 0 };
        };

        bool Upload(ByteView pixels);

    private:
        bool m_Loaded{ false };
    };
}