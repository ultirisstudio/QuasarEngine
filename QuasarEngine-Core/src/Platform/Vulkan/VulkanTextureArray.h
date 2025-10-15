#pragma once

#include "VulkanTypes.h"

#include <QuasarEngine/Resources/TextureArray.h>
#include <vulkan/vulkan.h>
#include <memory>

namespace QuasarEngine {

    class VulkanImage;

    class VulkanTextureArray : public TextureArray {
    public:
        explicit VulkanTextureArray(const TextureSpecification& specification);
        ~VulkanTextureArray() override;

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
        bool Upload(ByteView pixels, uint32_t layers);

        bool m_Loaded{ false };
    };
}