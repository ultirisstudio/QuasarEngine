#include <quasar/renderer/Image.h>

#include <quasar/io/File.h>
#include <quasar/log/Log.h>

#include <stb_image.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Quasar
{
    namespace
    {
        constexpr int Channels = 4;

        struct PixelsDeleter
        {
            void operator()(stbi_uc* pixels) const { stbi_image_free(pixels); }
        };

        std::optional<Image> Decode(std::span<const std::byte> encoded, const std::string& name)
        {
            if (encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            {
                QUASAR_ENGINE_ERROR("Cannot decode the image {}: it is larger than 2 GB", name);
                return std::nullopt;
            }

            int width = 0;
            int height = 0;
            int fileChannels = 0;
            // stb reads the bytes as unsigned characters, which hold the same values
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            const auto* data = reinterpret_cast<const stbi_uc*>(encoded.data());
            const std::unique_ptr<stbi_uc, PixelsDeleter> pixels(
                stbi_load_from_memory(data, static_cast<int>(encoded.size()), &width, &height, &fileChannels, Channels));
            if (pixels == nullptr)
            {
                QUASAR_ENGINE_ERROR("Cannot decode the image {}: {}", name, stbi_failure_reason());
                return std::nullopt;
            }

            Image image{.Width = static_cast<std::uint32_t>(width), .Height = static_cast<std::uint32_t>(height), .Pixels = {}};
            const std::size_t size = std::size_t{image.Width} * image.Height * Channels;
            const std::span<const std::byte> bytes = std::as_bytes(std::span<const stbi_uc>(pixels.get(), size));
            image.Pixels.assign(bytes.begin(), bytes.end());
            return image;
        }
    }

    std::optional<Image> Image::FromFile(const std::filesystem::path& file)
    {
        const std::optional<std::vector<std::byte>> encoded = ReadBinaryFile(file);
        if (!encoded)
        {
            return std::nullopt;
        }
        return Decode(*encoded, "'" + file.generic_string() + "'");
    }

    std::optional<Image> Image::FromMemory(std::span<const std::byte> encoded)
    {
        return Decode(encoded, "in memory");
    }
}
