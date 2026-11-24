#pragma once

/// \file
/// \brief The effects applied to the image of a scene after it is drawn, such as bloom or antialiasing.
/// \author Kurogenshi
/// \date 2026-11-21

#include <quasar/core/Base.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GpuProfiler.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/ToneMapper.h>

#include <glm/vec2.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Quasar
{
    /// @brief When a post-processing effect runs, compared to the tone mapping.
    enum class PostProcessStage : std::uint8_t
    {
        /// Before the tone mapping: the effect reads and writes linear colors, which can go above 1, in framebuffers of
        /// format FramebufferFormat::Rgba16F. For effects that work on light, such as bloom or depth of field.
        BeforeToneMapping,

        /// After the tone mapping: the effect reads and writes the sRGB colors of the screen, between 0 and 1, in
        /// framebuffers of format FramebufferFormat::Rgba8. For effects that work on the final image, such as FXAA.
        AfterToneMapping
    };

    /// @brief What a post-processing effect reads.
    struct PostProcessInput
    {
        const Framebuffer* Source = nullptr; ///< The image to process, in its first color attachment, never multisampled when read.
        const Framebuffer* Scene = nullptr;  ///< The framebuffer that the scene was drawn into, with its depth.
        const Camera* View = nullptr;        ///< The camera that the scene was drawn from.
        glm::uvec2 Size{0};                  ///< The size of the image, in pixels.
    };

    /// @brief An effect applied to the image of a scene, such as bloom. See PostProcessStack.
    ///
    /// An effect is a class derived from this one, which gives its name and its stage to the constructor and draws itself
    /// in Apply(). It usually draws with a FullscreenPass.
    class PostProcessEffect
    {
    public:
        virtual ~PostProcessEffect() = default;

        PostProcessEffect(const PostProcessEffect&) = delete;
        PostProcessEffect& operator=(const PostProcessEffect&) = delete;
        PostProcessEffect(PostProcessEffect&&) = delete;
        PostProcessEffect& operator=(PostProcessEffect&&) = delete;

        /// @brief Returns the name of the effect, for example to show it in an options menu.
        const std::string& GetName() const { return m_Name; }

        /// @brief Returns when the effect runs, compared to the tone mapping.
        PostProcessStage GetStage() const { return m_Stage; }

        /// @brief Returns whether the effect runs. An effect is enabled when it is created.
        bool IsEnabled() const { return m_Enabled; }

        /// @brief Enables or disables the effect, from the next PostProcessStack::Apply(). A disabled effect costs
        /// nothing.
        void SetEnabled(bool enabled) { m_Enabled = enabled; }

        /// @brief Draws the effect.
        /// @param input The image to process and what the effect may read about the scene.
        /// @param target The framebuffer to write, of the size of the image, or nullptr for the window. The effect must
        /// write every pixel of it.
        virtual void Apply(const PostProcessInput& input, Framebuffer* target) = 0;

    protected:
        /// @brief Creates an enabled effect.
        PostProcessEffect(std::string name, PostProcessStage stage) : m_Name(std::move(name)), m_Stage(stage) {}

    private:
        std::string m_Name;
        PostProcessStage m_Stage;
        bool m_Enabled = true;
    };

    /// @brief The post-processing effects of a view, applied in order between the image of the scene and the target.
    ///
    /// Apply() runs the enabled effects of the stage PostProcessStage::BeforeToneMapping in the order they were added,
    /// then the ToneMapper, then the enabled effects of the stage PostProcessStage::AfterToneMapping. Each effect reads
    /// the image written by the previous step. The stack keeps two framebuffers for each stage and alternates between
    /// them, and the last step writes into the target. Without effects, the tone mapper writes directly into the target.
    class PostProcessStack
    {
    public:
        /// @brief Creates a stack without effects, which draws with a context that must outlive it.
        /// @return The stack, or nullptr if its tone mapper or its framebuffers cannot be created. The reason is logged
        /// as an engine error.
        static Scope<PostProcessStack> Create(GraphicsContext& context);

        ~PostProcessStack() = default;

        PostProcessStack(const PostProcessStack&) = delete;
        PostProcessStack& operator=(const PostProcessStack&) = delete;
        PostProcessStack(PostProcessStack&&) = delete;
        PostProcessStack& operator=(PostProcessStack&&) = delete;

        /// @brief Adds an effect after the others of its stage, and takes ownership of it.
        /// @return The effect, or nullptr for no effect, which is refused with an engine error, for example when its
        /// Create() failed.
        template <typename T>
        T* Add(Scope<T> effect)
        {
            T* added = effect.get();
            return AddEffect(std::move(effect)) ? added : nullptr;
        }

        /// @brief Removes an effect of the stack and destroys it.
        /// @return False if the effect is not in the stack.
        bool Remove(const PostProcessEffect& effect);

        /// @brief Returns the first effect of a type, or nullptr if the stack has none.
        template <typename T>
        T* Find() const
        {
            for (const Scope<PostProcessEffect>& effect : m_Effects)
            {
                if (auto* found = dynamic_cast<T*>(effect.get()); found != nullptr)
                {
                    return found;
                }
            }
            return nullptr;
        }

        /// @brief Returns the effects, in the order they were added.
        const std::vector<Scope<PostProcessEffect>>& GetEffects() const { return m_Effects; }

        /// @brief Applies the effects and the tone mapping to the image of a scene.
        ///
        /// Each effect and the tone mapping are a section of the profiler of the context, named after the effect and
        /// "Tone mapping". See GpuProfiler.
        /// @param scene The framebuffer of the scene, in linear colors in its first color attachment, of format Rgba16F.
        /// @param target The framebuffer to draw into, of format Rgba8 or Rgba16F and of the size of the scene, or
        /// nullptr for the window. The window is bound again afterwards.
        /// @param camera The camera that the scene was drawn from.
        /// @param exposure The factor of the linear colors, applied before the tonemapping.
        /// @param tonemapping How the colors above 1 are brought to the screen.
        void Apply(const Framebuffer& scene, Framebuffer* target, const Camera& camera, float exposure, Tonemapping tonemapping);

    private:
        using Buffers = std::array<Scope<Framebuffer>, 2>;

        PostProcessStack(GpuProfiler& profiler, Scope<ToneMapper> toneMapper, Buffers linear, Buffers display);

        bool AddEffect(Scope<PostProcessEffect> effect);
        void GatherEnabled(PostProcessStage stage);
        const Framebuffer* ApplyStage(PostProcessInput& input, Buffers& buffers, std::size_t first, bool toTarget, Framebuffer* target);

        GpuProfiler* m_Profiler;
        Scope<ToneMapper> m_ToneMapper;
        Buffers m_LinearBuffers;
        Buffers m_DisplayBuffers;
        std::vector<Scope<PostProcessEffect>> m_Effects;
        std::vector<PostProcessEffect*> m_Enabled;
    };
}
