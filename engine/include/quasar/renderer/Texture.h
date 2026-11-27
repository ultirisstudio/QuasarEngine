#pragma once

/// \file
/// \brief 2D textures: images that the shaders read, such as the colors of a material or a sprite.
/// \author Kurogenshi
/// \date 2026-11-18

#include <imgui.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace Quasar
{
    /// @brief Format of the pixels of a texture.
    enum class TextureFormat : std::uint8_t
    {
        R8,     ///< One channel of 8 bits, for example a mask. The shaders read it in the red channel.
        Rgba8,  ///< Red, green, blue and alpha, with 8 bits each, read as they are stored.
        Srgba8, ///< Like Rgba8, but the shaders read red, green and blue converted from sRGB to linear values, as the
                ///< lighting needs for the colors painted in an image file.
        Rgba16F ///< Red, green, blue and alpha, with a 16-bit float each, for values above 1 such as HDR images.
    };

    /// @brief Returns the size of one pixel in the data that fills a texture of this format, in bytes: 1 for R8, 4 for
    /// Rgba8 and Srgba8, and 16 for Rgba16F, whose pixels are given as four 32-bit floats.
    std::uint32_t GetTexturePixelSize(TextureFormat format);

    /// @brief The textures of 1 x 1 pixel that a GraphicsContext makes, for example for a material texture that is not
    /// set. See GraphicsContext::GetDefaultTexture().
    enum class DefaultTexture : std::uint8_t
    {
        White,     ///< White, in TextureFormat::Srgba8: a color or a mask that changes nothing.
        FlatNormal ///< The normal (0, 0, 1) of a normal map, in TextureFormat::Rgba8: the surface keeps its normal.
    };

    /// @brief How a texture is read between its pixels.
    enum class TextureFilter : std::uint8_t
    {
        Nearest, ///< The closest pixel, for pixel art.
        Linear   ///< A blend of the closest pixels, and of the closest mipmaps, for smooth images.
    };

    /// @brief How a texture is read outside of the texture coordinates 0 to 1.
    enum class TextureWrap : std::uint8_t
    {
        Repeat,         ///< The image repeats.
        MirroredRepeat, ///< The image repeats, mirrored every other time.
        ClampToEdge     ///< The pixels of the edges continue.
    };

    /// @brief Settings of a texture.
    struct TextureSpecification
    {
        std::uint32_t Width = 1;  ///< The width, in pixels.
        std::uint32_t Height = 1; ///< The height, in pixels.
        /// The format of the pixels: Srgba8 for colors, the default, or Rgba8 for data that the shaders read as they are
        /// stored, such as a normal map or a mask.
        TextureFormat Format = TextureFormat::Srgba8;
        TextureFilter Filter = TextureFilter::Linear; ///< How the texture is read between its pixels.
        TextureWrap Wrap = TextureWrap::Repeat;       ///< How the texture is read outside of the coordinates 0 to 1.
        bool Mipmaps = true;                          ///< Keeps smaller copies of the image, which avoid flickering when the texture is far.

        /// How many samples a read takes along the direction in which the texture is seen at a grazing angle, such as a
        /// ground far away, so that it stays sharp without flickering. It goes from 1, for none, to
        /// GraphicsContext::GetMaxAnisotropy(), to which a higher value is lowered. It is used only with
        /// TextureFilter::Linear and mipmaps.
        float Anisotropy = 16.0f;
    };

    /// @brief An image that the shaders read with a `sampler2D`.
    ///
    /// A texture is created with GraphicsContext::CreateTexture2D(), for example from an Image. Its first row of
    /// pixels is the top of the image, at the texture coordinate v = 0, as in glTF and Vulkan, so the images of files
    /// are never flipped. Its size and format never change: SetData() replaces all its pixels.
    class Texture2D
    {
    public:
        virtual ~Texture2D() = default;

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;
        Texture2D(Texture2D&&) = delete;
        Texture2D& operator=(Texture2D&&) = delete;

        /// @brief Returns true if a texture can be created from the specification, whatever the graphics API. Its size
        /// must not be 0. The reason a specification is refused is logged as an engine error.
        static bool IsValid(const TextureSpecification& specification);

        /// @brief Replaces all the pixels, and updates the mipmaps.
        /// @param pixels Width * Height pixels of GetTexturePixelSize() bytes, from the top row. Data of another size is
        /// refused with an engine error, and the texture is not changed.
        void SetData(std::span<const std::byte> pixels);

        /// @brief Makes the next draws read the texture from a `layout(binding = N) uniform sampler2D`.
        /// @param slot The N of the sampler. A slot that the graphics API does not have is refused with an engine error.
        virtual void Bind(std::uint32_t slot) = 0;

        /// @brief Returns the texture that ImGui::Image() draws, upright with the default texture coordinates.
        virtual ImTextureID GetImGuiTexture() const = 0;

        /// @brief Returns the specification of the texture.
        const TextureSpecification& GetSpecification() const { return m_Specification; }

        /// @brief Returns the size of all the pixels of the texture, in bytes, which SetData() expects.
        std::size_t GetDataSize() const;

    protected:
        /// @brief Keeps the specification, which must be valid. See IsValid().
        explicit Texture2D(const TextureSpecification& specification);

        /// @brief Copies all the pixels into the texture and updates the mipmaps. The data has the right size.
        virtual void Write(std::span<const std::byte> pixels) = 0;

    private:
        TextureSpecification m_Specification;
    };
}
