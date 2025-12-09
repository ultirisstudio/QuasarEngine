#pragma once

#include <QuasarEngine/Resources/Texture2D.h>

namespace QuasarEngine
{
    class DirectXTexture2D : public Texture2D {
    public:
        explicit DirectXTexture2D(const TextureSpecification& specification);
        ~DirectXTexture2D() override;

        TextureHandle GetHandle() const noexcept override;
        bool IsLoaded() const noexcept override;

        bool LoadFromPath(const std::string& path) override;
        bool LoadFromMemory(ByteView data) override;
        bool LoadFromData(ByteView data) override;

        glm::vec4 Sample(const glm::vec2& uv) const override;

        void Bind(int index = 0) const override;
        void Unbind() const override;

        void GenerateMips() override;
    };
}
