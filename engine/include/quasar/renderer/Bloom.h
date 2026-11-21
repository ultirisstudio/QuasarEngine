#pragma once

/// \file
/// \brief The glow around the bright parts of an image.
/// \author Kurogenshi
/// \date 2026-11-21

#include <quasar/core/Base.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/FullscreenPass.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/PostProcessing.h>
#include <quasar/renderer/Shader.h>

#include <glm/vec2.hpp>

#include <array>
#include <cstddef>

namespace Quasar
{
    /// @brief Settings of a Bloom.
    struct BloomSettings
    {
        float Threshold = 1.0f; ///< The linear brightness from which a pixel glows. 0 makes every pixel glow.
        float SoftKnee = 0.5f;  ///< From 0, which cuts sharply at the threshold, to 1, which fades the glow in below it.
        float Intensity = 0.1f; ///< The share of the glow added to the image. 0 adds nothing.
        float Radius = 1.0f;    ///< The spread of the glow. Above 2, the glow shows blocks.
    };

    /// @brief Makes the bright parts of an image glow over their surroundings, like a camera lens or the eye.
    ///
    /// The effect runs before the tone mapping, on linear colors. The parts brighter than BloomSettings::Threshold are
    /// blurred at 6 sizes, from half the image down, and the blur is added to the image. This is the method of Call of
    /// Duty: Advanced Warfare (Jimenez, 2014): a 13 samples filter halves the image at each step, then a 3 x 3 tent
    /// filter brings each step back up. The first step weights each sample by its brightness, so that one very bright
    /// pixel does not flicker.
    class Bloom final : public PostProcessEffect
    {
    public:
        /// @brief The most steps of the blur, each half the size of the previous one.
        static constexpr std::size_t MaxLevels = 6;

        /// @brief Creates a bloom that draws with a context, which must outlive it.
        /// @return The bloom, or nullptr if its shaders cannot be read from the engine resources or if its pipelines or
        /// its framebuffers cannot be created. The reason is logged as an engine error.
        static Scope<Bloom> Create(GraphicsContext& context, const BloomSettings& settings = {});

        /// @brief Returns the settings.
        const BloomSettings& GetSettings() const { return m_Settings; }

        /// @brief Replaces the settings, used from the next Apply(). Negative values count as 0, and a soft knee above 1
        /// as 1.
        void SetSettings(const BloomSettings& settings) { m_Settings = settings; }

        /// @brief Returns the number of steps of the blur at the last Apply(): fewer than MaxLevels for an image too
        /// small to be halved that many times.
        std::size_t GetLevelCount() const { return m_LevelCount; }

        /// @brief Adds the glow of the image to it. See PostProcessEffect::Apply().
        void Apply(const PostProcessInput& input, Framebuffer* target) override;

    private:
        struct Program
        {
            Scope<Quasar::Shader> Shader;
            Scope<Quasar::Pipeline> Pipeline;
        };

        struct Resources
        {
            Scope<FullscreenPass> Pass;
            Program Downsample;
            Program Upsample;
            Program Composite;
            std::array<Scope<Framebuffer>, MaxLevels> Levels;
        };

        Bloom(const BloomSettings& settings, Resources resources);

        void ResizeLevels(glm::uvec2 size);

        BloomSettings m_Settings;
        Resources m_Resources;
        std::size_t m_LevelCount = 0;
    };
}
