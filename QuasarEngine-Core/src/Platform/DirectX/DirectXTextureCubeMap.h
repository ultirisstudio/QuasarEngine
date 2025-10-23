#pragma once

#include <QuasarEngine/Resources/TextureCubeMap.h>

namespace QuasarEngine
{
    class DirectXTextureCubeMap : public TextureCubeMap {
    public:
        explicit DirectXTextureCubeMap(const TextureSpecification& specification);
        ~DirectXTextureCubeMap() override;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(0); }
        bool IsLoaded() const noexcept override { return false; }

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView data) override;

        bool LoadFaceFromPath(Face face, const std::string& path) override;
        bool LoadFaceFromMemory(Face face, ByteView data) override;
        bool LoadFaceFromData(Face face, ByteView data, uint32_t w, uint32_t h, uint32_t channels) override;

        void Bind(int index = 0) const override;
        void Unbind() const override;

        void GenerateMips() override {}
    };
}