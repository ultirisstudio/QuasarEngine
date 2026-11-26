#include <quasar/renderer/GraphicsContext.h>

#include <quasar/log/Log.h>
#include <quasar/renderer/Image.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace Quasar
{
    Scope<Texture2D> GraphicsContext::LoadTexture2D(const std::filesystem::path& file, TextureSpecification settings)
    {
        if (settings.Format != TextureFormat::Rgba8 && settings.Format != TextureFormat::Srgba8)
        {
            QUASAR_ENGINE_ERROR("Cannot load the texture '{}': an image fills only a texture of format Rgba8 or Srgba8", file.generic_string());
            return nullptr;
        }
        const std::optional<Image> image = Image::FromFile(file);
        if (!image)
        {
            return nullptr;
        }
        settings.Width = image->Width;
        settings.Height = image->Height;
        return CreateTexture2D(settings, image->Pixels);
    }

    Texture2D* GraphicsContext::GetWhiteTexture()
    {
        if (m_WhiteTexture == nullptr)
        {
            constexpr std::array<std::uint8_t, 4> pixel{255, 255, 255, 255};
            m_WhiteTexture = CreateTexture2D({.Width = 1, .Height = 1, .Mipmaps = false}, std::as_bytes(std::span(pixel)));
        }
        return m_WhiteTexture.get();
    }

    GpuProfiler& GraphicsContext::GetProfiler()
    {
        if (m_Profiler == nullptr)
        {
            m_Profiler = CreateScope<GpuProfiler>([this] { return CreateTimestampQuery(); });
        }
        return *m_Profiler;
    }

    void GraphicsContext::EndFrame()
    {
        m_FrameStatistics = m_CurrentStatistics;
        m_CurrentStatistics = {};
    }

    void GraphicsContext::ReleaseResources()
    {
        m_WhiteTexture.reset();
        m_Profiler.reset();
    }
}
