#include <quasar/renderer/AmbientOcclusion.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>

#include <glm/mat4x4.hpp>

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace Quasar
{
    namespace
    {
        bool CreateProgram(GraphicsContext& context, std::string_view file, Scope<Shader>& shader, Scope<Pipeline>& pipeline)
        {
            const std::optional<ShaderSpecification> specification = ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / file);
            if (!specification)
            {
                return false;
            }
            shader = context.CreateShader(*specification);
            if (shader == nullptr)
            {
                return false;
            }
            pipeline = context.CreatePipeline({.Shader = shader.get(), .DepthTest = false, .DepthWrite = false, .Culling = CullMode::None});
            return pipeline != nullptr;
        }

        Scope<Framebuffer> CreateImage(GraphicsContext& context, std::vector<FramebufferFormat> formats, bool& created)
        {
            Scope<Framebuffer> image = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = std::move(formats)});
            created = created && image != nullptr;
            return image;
        }
    }

    AmbientOcclusion::AmbientOcclusion(GpuProfiler& profiler, Resources resources)
        : m_Profiler(&profiler), m_Pass(std::move(resources.Pass)), m_Occlusion(std::move(resources.Occlusion)), m_Blur(std::move(resources.Blur)),
          m_Upsample(std::move(resources.Upsample)), m_Raw(std::move(resources.Raw)), m_Blurred(std::move(resources.Blurred)),
          m_Smoothed(std::move(resources.Smoothed)), m_Result(std::move(resources.Result))
    {
    }

    Scope<AmbientOcclusion> AmbientOcclusion::Create(GraphicsContext& context)
    {
        Resources resources;
        resources.Pass = FullscreenPass::Create(context);
        bool created = resources.Pass != nullptr;
        created = CreateProgram(context, "AmbientOcclusion.glsl", resources.Occlusion.Shader, resources.Occlusion.Pipeline) && created;
        created = CreateProgram(context, "AmbientOcclusionBlur.glsl", resources.Blur.Shader, resources.Blur.Pipeline) && created;
        created = CreateProgram(context, "AmbientOcclusionUpsample.glsl", resources.Upsample.Shader, resources.Upsample.Pipeline) && created;
        resources.Raw = CreateImage(context, {FramebufferFormat::Red8, FramebufferFormat::Red32F}, created);
        resources.Blurred = CreateImage(context, {FramebufferFormat::Red8}, created);
        resources.Smoothed = CreateImage(context, {FramebufferFormat::Red8}, created);
        resources.Result = CreateImage(context, {FramebufferFormat::Red8}, created);
        if (!created)
        {
            QUASAR_ENGINE_ERROR("Cannot create an ambient occlusion: its shaders, its pipelines or its framebuffers cannot be created");
            return nullptr;
        }
        return Scope<AmbientOcclusion>(new AmbientOcclusion(context.GetProfiler(), std::move(resources)));
    }

    void AmbientOcclusion::Compute(const Framebuffer& depth, const Camera& camera, const AmbientOcclusionSettings& settings)
    {
        const FramebufferSpecification& size = depth.GetSpecification();
        const std::uint32_t scale = settings.HalfResolution ? 2 : 1;
        const std::uint32_t width = (size.Width + scale - 1) / scale;
        const std::uint32_t height = (size.Height + scale - 1) / scale;
        m_Raw->Resize(width, height);
        m_Blurred->Resize(width, height);
        m_Smoothed->Resize(width, height);
        m_Result->Resize(size.Width, size.Height);
        const glm::mat4& projection = camera.GetProjection();

        {
            const GpuSection section(m_Profiler, "Occlusion");
            Shader& occlusion = *m_Occlusion.Shader;
            occlusion.SetMat4("u_Projection", projection);
            occlusion.SetFloat("u_Radius", settings.Radius);
            occlusion.SetFloat("u_Bias", std::max(settings.Bias, 0.0f));
            occlusion.SetFloat("u_Intensity", std::max(settings.Intensity, 0.0f));
            occlusion.SetInt("u_SampleCount", static_cast<int>(std::clamp(settings.Samples, 1U, MaxSamples)));
            occlusion.SetInt("u_Scale", static_cast<int>(scale));
            depth.BindDepthValues(0);
            m_Pass->Draw(*m_Occlusion.Pipeline, m_Raw.get());
        }
        {
            const GpuSection section(m_Profiler, "Blur");
            const int radius = settings.HalfResolution ? 1 : 2;
            Blur(*m_Raw, false, radius, m_Blurred.get());
            Blur(*m_Blurred, true, radius, settings.HalfResolution ? m_Smoothed.get() : m_Result.get());
        }
        if (settings.HalfResolution)
        {
            const GpuSection section(m_Profiler, "Upsample");
            m_Upsample.Shader->SetMat4("u_Projection", projection);
            m_Smoothed->BindColorTexture(0, 0);
            m_Raw->BindColorTexture(1, 1);
            depth.BindDepthValues(2);
            m_Pass->Draw(*m_Upsample.Pipeline, m_Result.get());
        }
    }

    void AmbientOcclusion::Blur(const Framebuffer& source, bool vertical, int radius, Framebuffer* target)
    {
        m_Blur.Shader->SetInt("u_Vertical", vertical ? 1 : 0);
        m_Blur.Shader->SetInt("u_Radius", radius);
        source.BindColorTexture(0, 0);
        m_Raw->BindColorTexture(1, 1);
        m_Pass->Draw(*m_Blur.Pipeline, target);
    }
}
