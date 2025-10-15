#pragma once

#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
    class DirectXTexture2D : public Texture2D {
    public:
        explicit DirectXTexture2D(const TextureSpecification& specification);
        ~DirectXTexture2D() override;

        TextureHandle GetHandle() const noexcept override { return static_cast<TextureHandle>(0); }
        bool IsLoaded() const noexcept override { return false; }

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView data) override;

        void Bind(int index = 0) const override;
        void Unbind() const override;
    };
}
