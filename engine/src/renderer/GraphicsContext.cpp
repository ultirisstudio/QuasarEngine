#include <quasar/renderer/GraphicsContext.h>

#include <quasar/log/Log.h>
#include <quasar/renderer/Image.h>

#include <optional>

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
}
