#include <quasar/renderer/ToneMapper.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>

#include <optional>
#include <utility>

namespace Quasar
{
    ToneMapper::ToneMapper(Scope<FullscreenPass> pass, Scope<Shader> shader, Scope<Pipeline> pipeline)
        : m_Pass(std::move(pass)), m_Shader(std::move(shader)), m_Pipeline(std::move(pipeline))
    {
    }

    Scope<ToneMapper> ToneMapper::Create(GraphicsContext& context)
    {
        const std::optional<ShaderSpecification> specification = ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / "ToneMapping.glsl");
        if (!specification)
        {
            QUASAR_ENGINE_ERROR("Cannot create a tone mapper: its shader cannot be read");
            return nullptr;
        }

        Scope<Shader> shader = context.CreateShader(*specification);
        Scope<Pipeline> pipeline;
        if (shader != nullptr)
        {
            pipeline = context.CreatePipeline({.Shader = shader.get(), .DepthTest = false, .DepthWrite = false, .Culling = CullMode::None});
        }
        Scope<FullscreenPass> pass = FullscreenPass::Create(context);
        if (pipeline == nullptr || pass == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a tone mapper: its pipeline cannot be created");
            return nullptr;
        }
        return Scope<ToneMapper>(new ToneMapper(std::move(pass), std::move(shader), std::move(pipeline)));
    }

    void ToneMapper::Apply(const Framebuffer& source, Framebuffer* target, float exposure, Tonemapping tonemapping)
    {
        m_Shader->SetFloat("u_Exposure", exposure);
        m_Shader->SetInt("u_Tonemapping", static_cast<int>(tonemapping));
        source.BindColorTexture(0, 0);
        m_Pass->Draw(*m_Pipeline, target);
    }
}
