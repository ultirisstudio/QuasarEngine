#pragma once

#include "VulkanTypes.h"

#include <QuasarEngine/Resources/TextureCubeMap.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <array>
#include <vector>

namespace QuasarEngine {

    class VulkanImage;

    class VulkanTextureCubeMap : public TextureCubeMap {
    public:
        explicit VulkanTextureCubeMap(const TextureSpecification& specification);
        ~VulkanTextureCubeMap() override;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(reinterpret_cast<uintptr_t>(descriptor)); }
        bool IsLoaded() const noexcept override { return m_Loaded; }

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView data) override;

        bool LoadFaceFromPath(Face face, const std::string& path) override;
        bool LoadFaceFromMemory(Face face, ByteView data) override;
        bool LoadFaceFromData(Face face, ByteView data, uint32_t w, uint32_t h, uint32_t channels) override;

        void Bind(int index = 0) const override;
        void Unbind() const override;

    public:
        std::unique_ptr<VulkanImage> image{};
        VkSampler        sampler{ VK_NULL_HANDLE };
        uint32_t         generation{ 0 };
        VkDescriptorSet  descriptor{ VK_NULL_HANDLE };

    private:
        bool UploadAllFaces(ByteView allFacesPixels);
        bool TryUploadIfComplete();

        static uint32_t FaceIndex(Face f) { return static_cast<uint32_t>(f); }
        static bool IsComplete(const std::array<bool, 6>& arr) {
            for (bool b : arr) if (!b) return false;
            return true;
        }

    private:
        bool m_Loaded{ false };

        std::array<std::vector<std::uint8_t>, 6> m_StagedFaces{};
        std::array<bool, 6> m_HasFace{};
        uint32_t m_FaceW{ 0 }, m_FaceH{ 0 }, m_FaceChannels{ 0 };
    };
}