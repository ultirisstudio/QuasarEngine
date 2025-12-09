#pragma once

#include <QuasarEngine/Resources/TextureArray.h>

namespace QuasarEngine
{
    class DirectXTextureArray : public TextureArray {
    public:
        explicit DirectXTextureArray(const TextureSpecification& specification);
        ~DirectXTextureArray() override;

        TextureHandle GetHandle() const noexcept override;
        bool IsLoaded() const noexcept override;

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView data) override;

        bool LoadFromFiles(const std::vector<std::string>& paths) override;

        void Bind(int index = 0) const override;
        void Unbind() const override;

        void GenerateMips() override;
    };
}