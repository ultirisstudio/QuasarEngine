#include <quasar/renderer/Texture.h>

#include <quasar/log/Log.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace Quasar
{
    std::uint32_t GetTexturePixelSize(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::R8:
                return 1;
            case TextureFormat::Rgba8:
            case TextureFormat::Srgba8:
                return 4;
            case TextureFormat::Rgba16F:
                return 16;
        }
        return 4;
    }

    Texture2D::Texture2D(const TextureSpecification& specification) : m_Specification(specification) {}

    bool Texture2D::IsValid(const TextureSpecification& specification)
    {
        if (specification.Width == 0 || specification.Height == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot create a texture of {}x{} pixels", specification.Width, specification.Height);
            return false;
        }
        return true;
    }

    std::size_t Texture2D::GetDataSize() const
    {
        return std::size_t{m_Specification.Width} * m_Specification.Height * GetTexturePixelSize(m_Specification.Format);
    }

    void Texture2D::SetData(std::span<const std::byte> pixels)
    {
        if (pixels.size() != GetDataSize())
        {
            QUASAR_ENGINE_ERROR("Cannot fill a texture of {}x{} pixels with {} bytes: it needs {}", m_Specification.Width, m_Specification.Height,
                                pixels.size(), GetDataSize());
            return;
        }
        Write(pixels);
    }
}
