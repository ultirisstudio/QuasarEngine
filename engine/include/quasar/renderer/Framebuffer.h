#pragma once

/// \file
/// \brief Framebuffers: images that the engine draws into instead of the window, for example the scene view of the
/// editor.
/// \author Kurogenshi
/// \date 2026-11-18

#include <glm/vec4.hpp>
#include <imgui.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace Quasar
{
    /// @brief Format of an attachment of a framebuffer.
    enum class FramebufferFormat : std::uint8_t
    {
        Rgba8,           ///< Color with 8 bits per channel.
        Rgba16F,         ///< Color with a 16-bit float per channel, for values above 1 such as HDR lighting.
        RedInteger,      ///< One signed 32-bit integer per pixel, for example the id of the entity drawn there.
        Depth24Stencil8, ///< Depth on 24 bits and stencil on 8 bits. A framebuffer has at most one depth attachment.

        /// Depth on a 32-bit float, without stencil, that the shaders can read as a shadow map, see BindDepthTexture(), or
        /// as values, see BindDepthValues(). A framebuffer has at most one depth attachment, and this one needs one sample
        /// per pixel.
        Depth32F
    };

    /// @brief Settings of a framebuffer.
    struct FramebufferSpecification
    {
        std::uint32_t Width = 1280; ///< Width of the attachments, in pixels.
        std::uint32_t Height = 720; ///< Height of the attachments, in pixels.

        /// The formats of the attachments. The color attachments are numbered from 0 in their order in this list,
        /// without counting the depth attachment, wherever it is.
        std::vector<FramebufferFormat> Attachments{FramebufferFormat::Rgba8, FramebufferFormat::Depth24Stencil8};

        /// Samples per pixel. With more than 1, the attachments are multisampled for antialiasing (MSAA), and Unbind()
        /// resolves them into images of one sample per pixel, which ReadInteger() and GetImGuiTexture() use.
        std::uint32_t Samples = 1;
    };

    /// @brief A set of images that the engine draws into instead of the window.
    ///
    /// A framebuffer is created with GraphicsContext::CreateFramebuffer() and drawn into between Bind() and Unbind().
    /// Its images are stored from the bottom row up, as in OpenGL: ImGui shows one upright with
    /// `ImGui::Image(framebuffer.GetImGuiTexture(0), size, ImVec2(0, 1), ImVec2(1, 0))`.
    ///
    /// The functions that take an attachment use the number of a color attachment, see
    /// FramebufferSpecification::Attachments. A function given an attachment that does not exist or that has the wrong
    /// format does nothing and logs an engine error.
    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;
        Framebuffer(Framebuffer&&) = delete;
        Framebuffer& operator=(Framebuffer&&) = delete;

        /// @brief Returns true if a framebuffer can be created from the specification, whatever the graphics API.
        ///
        /// Its size must not be 0, it must have at least one attachment and at most one depth attachment, and at least
        /// one sample per pixel. The reason a specification is refused is logged as an engine error.
        static bool IsValid(const FramebufferSpecification& specification);

        /// @brief Makes the next draws go into the framebuffer, on its whole area.
        virtual void Bind() = 0;

        /// @brief Ends the draws into the framebuffer. With several samples per pixel, it also resolves the
        /// attachments. GraphicsContext::BindWindow() then makes the next draws go into the window.
        virtual void Unbind() = 0;

        /// @brief Changes the size of the attachments, which loses their content. A size of 0, for example while a
        /// panel is collapsed, is ignored, and a size larger than the graphics API allows is refused with an engine error.
        virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;

        /// @brief Fills a color attachment of format Rgba8 or Rgba16F with a color.
        virtual void ClearColor(std::uint32_t attachment, glm::vec4 color) = 0;

        /// @brief Fills a color attachment of format RedInteger with a value.
        virtual void ClearInteger(std::uint32_t attachment, int value) = 0;

        /// @brief Fills the depth attachment with a depth and a stencil value.
        virtual void ClearDepthStencil(float depth = 1.0f, std::uint8_t stencil = 0) = 0;

        /// @brief Reads one pixel of a color attachment of format RedInteger, for example to find the entity under the
        /// mouse.
        /// @param attachment The number of the color attachment.
        /// @param x The column, from the left edge.
        /// @param y The row, from the top edge, like the position of the mouse.
        /// @return The value, or nothing if the pixel is outside of the framebuffer or if the attachment is not an integer
        /// attachment. With several samples per pixel, the value comes from the last Unbind().
        virtual std::optional<int> ReadInteger(std::uint32_t attachment, std::uint32_t x, std::uint32_t y) = 0;

        /// @brief Makes the shaders read a color attachment of format Rgba8 or Rgba16F from a texture slot, for example
        /// to process the image of a first pass in a second one. With several samples per pixel, the image comes from
        /// the last Unbind(). Another attachment is refused with an engine error.
        /// @param attachment The number of the color attachment.
        /// @param slot The N of the `layout(binding = N) uniform sampler2D` of the shader.
        virtual void BindColorTexture(std::uint32_t attachment, std::uint32_t slot) const = 0;

        /// @brief Makes the shaders read the depth attachment of format Depth32F from a texture slot, as a
        /// `sampler2DShadow`, for example a shadow map. A read compares a depth with the stored one, and gives 1 where it
        /// is closer or equal and 0 where it is farther, blended between the closest pixels. Outside of the framebuffer, a
        /// read gives 1. A framebuffer without such an attachment is refused with an engine error.
        /// @param slot The N of the `layout(binding = N) uniform sampler2DShadow` of the shader.
        virtual void BindDepthTexture(std::uint32_t slot) const = 0;

        /// @brief Makes the shaders read the depth attachment of format Depth32F from a texture slot as values, from 0 at
        /// the near plane to 1 at the far plane, with a `sampler2D`, for example to find the positions of the surfaces
        /// of a scene. A read gives the value of the closest pixel, without blending, and the edge pixels outside of the
        /// framebuffer. A framebuffer without such an attachment is refused with an engine error.
        /// @param slot The N of the `layout(binding = N) uniform sampler2D` of the shader.
        virtual void BindDepthValues(std::uint32_t slot) const = 0;

        /// @brief Returns the texture that ImGui::Image() draws for a color attachment of format Rgba8 or Rgba16F, or 0
        /// for another attachment. With several samples per pixel, the image comes from the last Unbind().
        virtual ImTextureID GetImGuiTexture(std::uint32_t attachment) const = 0;

        /// @brief Returns the specification of the framebuffer, with its current size.
        const FramebufferSpecification& GetSpecification() const { return m_Specification; }

    protected:
        /// @brief Keeps the specification, which must be valid. See IsValid().
        explicit Framebuffer(FramebufferSpecification specification);

        /// @brief Changes the size kept in the specification.
        void SetSize(std::uint32_t width, std::uint32_t height);

    private:
        FramebufferSpecification m_Specification;
    };
}
