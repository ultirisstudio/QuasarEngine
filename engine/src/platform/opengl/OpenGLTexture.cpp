#include <platform/opengl/OpenGLTexture.h>

#include <quasar/log/Log.h>

#include <glad/gl.h>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Quasar
{
    namespace
    {
        struct PixelFormat
        {
            GLenum Internal = GL_RGBA8;
            GLenum Format = GL_RGBA;
            GLenum Type = GL_UNSIGNED_BYTE;
        };

        PixelFormat GetPixelFormat(TextureFormat format)
        {
            switch (format)
            {
                case TextureFormat::R8:
                    return {.Internal = GL_R8, .Format = GL_RED, .Type = GL_UNSIGNED_BYTE};
                case TextureFormat::Rgba8:
                    return {.Internal = GL_RGBA8, .Format = GL_RGBA, .Type = GL_UNSIGNED_BYTE};
                case TextureFormat::Srgba8:
                    return {.Internal = GL_SRGB8_ALPHA8, .Format = GL_RGBA, .Type = GL_UNSIGNED_BYTE};
                case TextureFormat::Rgba16F:
                    return {.Internal = GL_RGBA16F, .Format = GL_RGBA, .Type = GL_FLOAT};
            }
            return {};
        }

        GLint GetMinFilter(TextureFilter filter, bool mipmaps)
        {
            if (filter == TextureFilter::Nearest)
            {
                return mipmaps ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
            }
            return mipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
        }

        GLint GetWrap(TextureWrap wrap)
        {
            switch (wrap)
            {
                case TextureWrap::Repeat:
                    return GL_REPEAT;
                case TextureWrap::MirroredRepeat:
                    return GL_MIRRORED_REPEAT;
                case TextureWrap::ClampToEdge:
                    return GL_CLAMP_TO_EDGE;
            }
            return GL_REPEAT;
        }

        std::uint32_t GetLimit(GLenum name)
        {
            GLint value = 0;
            glGetIntegerv(name, &value);
            return static_cast<std::uint32_t>(std::max(value, 0));
        }
    }

    OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& specification, std::uint32_t slotCount)
        : Texture2D(specification),
          m_Levels(specification.Mipmaps ? static_cast<std::uint32_t>(std::bit_width(std::max(specification.Width, specification.Height))) : 1),
          m_SlotCount(slotCount)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &m_Texture);
        glTextureStorage2D(m_Texture, static_cast<GLsizei>(m_Levels), GetPixelFormat(specification.Format).Internal, static_cast<GLsizei>(specification.Width),
                           static_cast<GLsizei>(specification.Height));
        glTextureParameteri(m_Texture, GL_TEXTURE_MIN_FILTER, GetMinFilter(specification.Filter, specification.Mipmaps));
        glTextureParameteri(m_Texture, GL_TEXTURE_MAG_FILTER, specification.Filter == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR);
        glTextureParameteri(m_Texture, GL_TEXTURE_WRAP_S, GetWrap(specification.Wrap));
        glTextureParameteri(m_Texture, GL_TEXTURE_WRAP_T, GetWrap(specification.Wrap));
    }

    OpenGLTexture2D::~OpenGLTexture2D()
    {
        glDeleteTextures(1, &m_Texture);
    }

    Scope<OpenGLTexture2D> OpenGLTexture2D::Create(const TextureSpecification& specification, std::span<const std::byte> pixels)
    {
        if (!IsValid(specification))
        {
            return nullptr;
        }

        const std::uint32_t maxSize = GetLimit(GL_MAX_TEXTURE_SIZE);
        if (specification.Width > maxSize || specification.Height > maxSize)
        {
            QUASAR_ENGINE_ERROR("Cannot create a texture of {}x{} pixels: OpenGL allows at most {}x{}", specification.Width, specification.Height, maxSize,
                                maxSize);
            return nullptr;
        }

        const std::size_t size = std::size_t{specification.Width} * specification.Height * GetTexturePixelSize(specification.Format);
        if (!pixels.empty() && pixels.size() != size)
        {
            QUASAR_ENGINE_ERROR("Cannot create a texture of {}x{} pixels from {} bytes: it needs {}", specification.Width, specification.Height, pixels.size(),
                                size);
            return nullptr;
        }

        Scope<OpenGLTexture2D> texture(new OpenGLTexture2D(specification, GetLimit(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS)));
        if (pixels.empty())
        {
            texture->Clear();
        }
        else
        {
            texture->Write(pixels);
        }
        return texture;
    }

    void OpenGLTexture2D::Bind(std::uint32_t slot)
    {
        if (slot >= m_SlotCount)
        {
            QUASAR_ENGINE_ERROR("Cannot bind a texture to the slot {}: OpenGL has the slots 0 to {}", slot, m_SlotCount - 1);
            return;
        }
        glBindTextureUnit(slot, m_Texture);
    }

    ImTextureID OpenGLTexture2D::GetImGuiTexture() const
    {
        return static_cast<ImTextureID>(m_Texture);
    }

    void OpenGLTexture2D::Write(std::span<const std::byte> pixels)
    {
        const TextureSpecification& specification = GetSpecification();
        const PixelFormat format = GetPixelFormat(specification.Format);

        // The rows of the pixels follow each other without padding, which OpenGL only assumes with an alignment of 1
        GLint alignment = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTextureSubImage2D(m_Texture, 0, 0, 0, static_cast<GLsizei>(specification.Width), static_cast<GLsizei>(specification.Height), format.Format,
                            format.Type, pixels.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

        if (m_Levels > 1)
        {
            glGenerateTextureMipmap(m_Texture);
        }
    }

    void OpenGLTexture2D::Clear()
    {
        const PixelFormat format = GetPixelFormat(GetSpecification().Format);
        for (std::uint32_t level = 0; level < m_Levels; ++level)
        {
            glClearTexImage(m_Texture, static_cast<GLint>(level), format.Format, format.Type, nullptr);
        }
    }
}
