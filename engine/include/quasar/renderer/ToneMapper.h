#pragma once

/// \file
/// \brief Brings the linear colors of an image, which can go above 1, to the sRGB colors of the screen.
/// \author Kurogenshi
/// \date 2026-11-20

#include <quasar/core/Base.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/FullscreenPass.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Shader.h>

#include <cstdint>

namespace Quasar
{
    /// @brief How the linear colors of a scene, which can go above 1, are brought between 0 and 1 for the screen.
    enum class Tonemapping : std::uint8_t
    {
        None,     ///< The colors above 1 are cut to 1, so bright areas lose their details.
        Reinhard, ///< Each channel c becomes c / (1 + c): soft, but the whites never reach 1.
        Aces      ///< The curve of the film industry standard ACES, as fitted by Krzysztof Narkowicz: contrasted, with soft whites.
    };

    /// @brief Draws an image in linear colors into a target in sRGB colors, with an exposure and a tonemapping.
    ///
    /// Each pixel of the first color attachment of the source is multiplied by the exposure, brought between 0 and 1 by
    /// the tonemapping, converted to sRGB and written over the same pixel of the target. The source and the target
    /// should have the same size.
    class ToneMapper
    {
    public:
        /// @brief Creates a tone mapper that draws with a context, which must outlive it.
        /// @return The tone mapper, or nullptr if its shader cannot be read from the engine resources or if its pipeline
        /// cannot be created. The reason is logged as an engine error.
        static Scope<ToneMapper> Create(GraphicsContext& context);

        ~ToneMapper() = default;

        ToneMapper(const ToneMapper&) = delete;
        ToneMapper& operator=(const ToneMapper&) = delete;
        ToneMapper(ToneMapper&&) = delete;
        ToneMapper& operator=(ToneMapper&&) = delete;

        /// @brief Draws a source into a target.
        /// @param source The framebuffer in linear colors, whose first color attachment, of format Rgba8 or Rgba16F, is
        /// read. With several samples per pixel, the image comes from its last Unbind().
        /// @param target The framebuffer to draw into, of format Rgba8 or Rgba16F, or nullptr for the window. The window
        /// is bound again afterwards.
        /// @param exposure The factor of the linear colors, applied before the tonemapping.
        /// @param tonemapping How the colors above 1 are brought to the screen.
        void Apply(const Framebuffer& source, Framebuffer* target, float exposure, Tonemapping tonemapping);

    private:
        ToneMapper(Scope<FullscreenPass> pass, Scope<Shader> shader, Scope<Pipeline> pipeline);

        Scope<FullscreenPass> m_Pass;
        Scope<Shader> m_Shader;
        Scope<Pipeline> m_Pipeline;
    };
}
