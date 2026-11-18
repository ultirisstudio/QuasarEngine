#pragma once

/// \file
/// \brief Images: the pixels of a PNG, JPEG, BMP or TGA file, decoded in memory.
/// \author Kurogenshi
/// \date 2026-11-18

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace Quasar
{
    /// @brief The pixels of a decoded image, ready to fill a Texture2D of format TextureFormat::Rgba8 or
    /// TextureFormat::Srgba8.
    ///
    /// Each pixel has 4 bytes, red, green, blue and alpha, and the rows go from the top of the image to its bottom, as
    /// in the file. An image without alpha is opaque, and a grayscale image has the same value in red, green and blue.
    struct Image
    {
        std::uint32_t Width = 0;       ///< The width, in pixels.
        std::uint32_t Height = 0;      ///< The height, in pixels.
        std::vector<std::byte> Pixels; ///< Width * Height * 4 bytes.

        /// @brief Decodes the image of a PNG, JPEG, BMP or TGA file.
        /// @return The image, or nothing if the file cannot be read or decoded. The reason is logged as an engine error.
        static std::optional<Image> FromFile(const std::filesystem::path& file);

        /// @brief Decodes the content of a PNG, JPEG, BMP or TGA file, already in memory.
        /// @return The image, or nothing if the data cannot be decoded. The reason is logged as an engine error.
        static std::optional<Image> FromMemory(std::span<const std::byte> encoded);
    };
}
