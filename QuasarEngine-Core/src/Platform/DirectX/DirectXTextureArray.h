#pragma once

#include <QuasarEngine/Resources/TextureArray.h>

namespace QuasarEngine
{
    class DirectXTextureArray : public TextureArray {
    public:
        explicit DirectXTextureArray(const TextureSpecification& specification);
        ~DirectXTextureArray() override;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(0); }
        bool IsLoaded() const noexcept override { return false; }

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView data) override;

        bool LoadFromFiles(const std::vector<std::string>& paths) override { return false; }

        void Bind(int index = 0) const override;
        void Unbind() const override;

        void GenerateMips() override {}
    };
}