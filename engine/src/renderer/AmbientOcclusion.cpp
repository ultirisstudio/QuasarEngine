#include <quasar/renderer/AmbientOcclusion.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>

#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <optional>
#include <string_view>
#include <utility>

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
    }

    AmbientOcclusion::AmbientOcclusion(Scope<FullscreenPass> pass, Program occlusion, Program blur, Scope<Framebuffer> raw, Scope<Framebuffer> result)
        : m_Pass(std::move(pass)), m_Occlusion(std::move(occlusion)), m_Blur(std::move(blur)), m_Raw(std::move(raw)), m_Result(std::move(result))
    {
    }

    Scope<AmbientOcclusion> AmbientOcclusion::Create(GraphicsContext& context)
    {
        Scope<FullscreenPass> pass = FullscreenPass::Create(context);
        Program occlusion;
        Program blur;
        bool created = pass != nullptr;
        created = CreateProgram(context, "AmbientOcclusion.glsl", occlusion.Shader, occlusion.Pipeline) && created;
        created = CreateProgram(context, "AmbientOcclusionBlur.glsl", blur.Shader, blur.Pipeline) && created;
        Scope<Framebuffer> raw = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Rgba8}});
        Scope<Framebuffer> result = context.CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::Rgba8}});
        if (!created || raw == nullptr || result == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create an ambient occlusion: its shaders, its pipelines or its framebuffers cannot be created");
            return nullptr;
        }
        return Scope<AmbientOcclusion>(new AmbientOcclusion(std::move(pass), std::move(occlusion), std::move(blur), std::move(raw), std::move(result)));
    }

    void AmbientOcclusion::Compute(const Framebuffer& depth, const Camera& camera, const AmbientOcclusionSettings& settings)
    {
        const FramebufferSpecification& size = depth.GetSpecification();
        m_Raw->Resize(size.Width, size.Height);
        m_Result->Resize(size.Width, size.Height);
        const glm::mat4 inverseProjection = glm::inverse(camera.GetProjection());

        Shader& occlusion = *m_Occlusion.Shader;
        occlusion.SetMat4("u_Projection", camera.GetProjection());
        occlusion.SetMat4("u_InverseProjection", inverseProjection);
        occlusion.SetFloat("u_Radius", settings.Radius);
        occlusion.SetFloat("u_Bias", std::max(settings.Bias, 0.0f));
        occlusion.SetFloat("u_Intensity", std::max(settings.Intensity, 0.0f));
        occlusion.SetInt("u_SampleCount", static_cast<int>(std::clamp(settings.Samples, 1U, MaxSamples)));
        depth.BindDepthValues(0);
        m_Pass->Draw(*m_Occlusion.Pipeline, m_Raw.get());

        m_Blur.Shader->SetMat4("u_InverseProjection", inverseProjection);
        m_Raw->BindColorTexture(0, 0);
        depth.BindDepthValues(1);
        m_Pass->Draw(*m_Blur.Pipeline, m_Result.get());
    }
}
