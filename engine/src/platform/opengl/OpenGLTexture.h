#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/Texture.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace Quasar
{
    class OpenGLDevice;

    class OpenGLTexture2D final : public Texture2D
    {
    public:
        static Scope<OpenGLTexture2D> Create(const TextureSpecification& specification, std::span<const std::byte> pixels, OpenGLDevice& device);

        ~OpenGLTexture2D() override;

        OpenGLTexture2D(const OpenGLTexture2D&) = delete;
        OpenGLTexture2D& operator=(const OpenGLTexture2D&) = delete;
        OpenGLTexture2D(OpenGLTexture2D&&) = delete;
        OpenGLTexture2D& operator=(OpenGLTexture2D&&) = delete;

        void Bind(std::uint32_t slot) override;
        ImTextureID GetImGuiTexture() const override;

    private:
        OpenGLTexture2D(const TextureSpecification& specification, OpenGLDevice& device);

        void Write(std::span<const std::byte> pixels) override;
        void Clear();

        OpenGLDevice* m_Device;
        std::uint32_t m_Texture = 0;
        std::uint32_t m_Levels = 1;
    };
}
