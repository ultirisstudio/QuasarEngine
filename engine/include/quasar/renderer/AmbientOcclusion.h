#pragma once

/// \file
/// \brief The ambient occlusion: how much of the light of the surroundings reaches each surface of a scene.
/// \author Kurogenshi
/// \date 2026-11-21

#include <quasar/core/Base.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/FullscreenPass.h>
#include <quasar/renderer/FrameProfiler.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Shader.h>

#include <cstdint>

namespace Quasar
{
    /// @brief Settings of the ambient occlusion.
    struct AmbientOcclusionSettings
    {
        bool Enabled = true;        ///< Whether the ambient occlusion darkens the scene.
        float Radius = 0.5f;        ///< The distance around each surface where other surfaces hide the light, in world units. 0 hides nothing.
        float Intensity = 1.0f;     ///< How dark the hidden parts get. 0 darkens nothing, and above 1 darkens more.
        float Bias = 0.025f;        ///< The distance, in world units, below which a surface does not hide another, against noise.
        std::uint32_t Samples = 16; ///< The points tested around each surface, from 1 to MaxSamples: more is smoother but slower.

        /// Whether the occlusion is computed on a quarter of the pixels, then brought back to the full size without
        /// blurring the edges of the objects. About 4 times faster, and the result is close.
        bool HalfResolution = true;

        /// The share of the ambient occlusion also applied to the direct lights, from 0, which darkens only the ambient
        /// light as in reality, to 1, which darkens every light for a stronger effect.
        float DirectLightingStrength = 0.0f;
    };

    /// @brief Computes the screen space ambient occlusion (SSAO) of a scene from its depth: the share of the light of the
    /// surroundings that reaches each pixel, lower in corners, in creases and where objects touch.
    ///
    /// For each pixel, the position and the normal of the surface are found from the depth. Points are then tested in
    /// the half sphere of AmbientOcclusionSettings::Radius above the surface, turned by a different angle for each pixel;
    /// a point behind another surface counts as hidden. A blur across, then down, smooths the noise of the angles without
    /// mixing surfaces at different depths: over 5 pixels, or over 3 computed pixels, so 6 pixels, at half resolution.
    ///
    /// With AmbientOcclusionSettings::HalfResolution, the occlusion is computed for one pixel of each block of 2 x 2,
    /// the closest or the farthest in turns so that both sides of an edge are kept. Each pixel of the result then mixes
    /// the closest computed pixels that are on its surface.
    ///
    /// The steps are sections of the profiler of the context: "Occlusion", "Blur" and, at half resolution, "Upsample".
    class AmbientOcclusion
    {
    public:
        /// @brief The most points tested around each surface.
        static constexpr std::uint32_t MaxSamples = 64;

        /// @brief Creates an ambient occlusion that draws with a context, which must outlive it.
        /// @return The ambient occlusion, or nullptr if its shaders cannot be read from the engine resources or if its
        /// pipelines or its framebuffers cannot be created. The reason is logged as an engine error.
        static Scope<AmbientOcclusion> Create(GraphicsContext& context);

        ~AmbientOcclusion() = default;

        AmbientOcclusion(const AmbientOcclusion&) = delete;
        AmbientOcclusion& operator=(const AmbientOcclusion&) = delete;
        AmbientOcclusion(AmbientOcclusion&&) = delete;
        AmbientOcclusion& operator=(AmbientOcclusion&&) = delete;

        /// @brief Computes the ambient occlusion of a scene into GetResult(), which takes the size of the depth.
        /// @param depth The framebuffer where the depth of the scene was drawn, with a FramebufferFormat::Depth32F
        /// attachment. The pixels at the far plane, where nothing was drawn, are not darkened.
        /// @param camera The camera that drew the depth.
        /// @param settings The radius, the intensity, the bias, the samples and the resolution. Enabled is not read here.
        void Compute(const Framebuffer& depth, const Camera& camera, const AmbientOcclusionSettings& settings);

        /// @brief Returns the framebuffer of the result, of the size of the depth, whose first color attachment, of format
        /// Red8, holds the share of the ambient light that reaches each pixel: 1 for none hidden, 0 for all.
        const Framebuffer& GetResult() const { return *m_Result; }

    private:
        struct Program
        {
            Scope<Quasar::Shader> Shader;
            Scope<Quasar::Pipeline> Pipeline;
        };

        struct Resources
        {
            Scope<FullscreenPass> Pass;
            Program Occlusion;
            Program Blur;
            Program Upsample;
            Scope<Framebuffer> Raw;
            Scope<Framebuffer> Blurred;
            Scope<Framebuffer> Smoothed;
            Scope<Framebuffer> Result;
        };

        AmbientOcclusion(FrameProfiler& profiler, Resources resources);

        void Blur(const Framebuffer& source, bool vertical, int radius, Framebuffer* target);

        FrameProfiler* m_Profiler;
        Scope<FullscreenPass> m_Pass;
        Program m_Occlusion;
        Program m_Blur;
        Program m_Upsample;
        Scope<Framebuffer> m_Raw;
        Scope<Framebuffer> m_Blurred;
        Scope<Framebuffer> m_Smoothed;
        Scope<Framebuffer> m_Result;
    };
}
