#include <quasar/renderer/PostProcessing.h>

#include <quasar/log/Log.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <utility>

namespace Quasar
{
    namespace
    {
        constexpr std::string_view ToneMappingSection = "Tone mapping";

        std::array<Scope<Framebuffer>, 2> CreateBuffers(GraphicsContext& context, FramebufferFormat format, bool& created)
        {
            std::array<Scope<Framebuffer>, 2> buffers;
            for (Scope<Framebuffer>& buffer : buffers)
            {
                buffer = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {format}});
                created = created && buffer != nullptr;
            }
            return buffers;
        }
    }

    PostProcessStack::PostProcessStack(FrameProfiler& profiler, Scope<ToneMapper> toneMapper, Buffers linear, Buffers display)
        : m_Profiler(&profiler), m_ToneMapper(std::move(toneMapper)), m_LinearBuffers(std::move(linear)), m_DisplayBuffers(std::move(display))
    {
    }

    Scope<PostProcessStack> PostProcessStack::Create(GraphicsContext& context)
    {
        Scope<ToneMapper> toneMapper = ToneMapper::Create(context);
        bool created = toneMapper != nullptr;
        Buffers linear = CreateBuffers(context, FramebufferFormat::Rgba16F, created);
        Buffers display = CreateBuffers(context, FramebufferFormat::Rgba8, created);
        if (!created)
        {
            QUASAR_ENGINE_ERROR("Cannot create a post-processing stack: its tone mapper or its framebuffers cannot be created");
            return nullptr;
        }
        return Scope<PostProcessStack>(new PostProcessStack(context.GetProfiler(), std::move(toneMapper), std::move(linear), std::move(display)));
    }

    bool PostProcessStack::AddEffect(Scope<PostProcessEffect> effect)
    {
        if (effect == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot add a post-processing effect that does not exist");
            return false;
        }
        m_Effects.push_back(std::move(effect));
        return true;
    }

    bool PostProcessStack::Remove(const PostProcessEffect& effect)
    {
        return std::erase_if(m_Effects, [&effect](const Scope<PostProcessEffect>& added) { return added.get() == &effect; }) > 0;
    }

    void PostProcessStack::GatherEnabled(PostProcessStage stage)
    {
        m_Enabled.clear();
        for (const Scope<PostProcessEffect>& effect : m_Effects)
        {
            if (effect->IsEnabled() && effect->GetStage() == stage)
            {
                m_Enabled.push_back(effect.get());
            }
        }
    }

    const Framebuffer* PostProcessStack::ApplyStage(PostProcessInput& input, Buffers& buffers, std::size_t first, bool toTarget, Framebuffer* target)
    {
        for (std::size_t index = 0; index < m_Enabled.size(); ++index)
        {
            Framebuffer* output = target;
            if (!toTarget || index + 1 < m_Enabled.size())
            {
                output = buffers.at((first + index) % buffers.size()).get();
                output->Resize(input.Size.x, input.Size.y);
            }
            const ProfilerSection section(m_Profiler, m_Enabled[index]->GetName());
            m_Enabled[index]->Apply(input, output);
            input.Source = output;
        }
        return input.Source;
    }

    void PostProcessStack::Apply(const Framebuffer& scene, Framebuffer* target, const Camera& camera, float exposure, Tonemapping tonemapping)
    {
        PostProcessInput input{.Source = &scene, .Scene = &scene, .View = &camera, .Size = {scene.GetSpecification().Width, scene.GetSpecification().Height}};

        GatherEnabled(PostProcessStage::BeforeToneMapping);
        const Framebuffer* linear = ApplyStage(input, m_LinearBuffers, 0, false, nullptr);

        GatherEnabled(PostProcessStage::AfterToneMapping);
        if (m_Enabled.empty())
        {
            const ProfilerSection section(m_Profiler, ToneMappingSection);
            m_ToneMapper->Apply(*linear, target, exposure, tonemapping);
            return;
        }
        Framebuffer& display = *m_DisplayBuffers[0];
        display.Resize(input.Size.x, input.Size.y);
        {
            const ProfilerSection section(m_Profiler, ToneMappingSection);
            m_ToneMapper->Apply(*linear, &display, exposure, tonemapping);
        }
        input.Source = &display;
        ApplyStage(input, m_DisplayBuffers, 1, true, target);
    }
}
