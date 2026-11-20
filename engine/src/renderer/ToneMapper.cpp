#include <quasar/renderer/ToneMapper.h>

#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>

#include <optional>
#include <utility>

namespace Quasar
{
    ToneMapper::ToneMapper(GraphicsContext& context, Scope<Shader> shader, Scope<Pipeline> pipeline, Scope<VertexArray> emptyVertexArray)
        : m_Context(&context), m_Shader(std::move(shader)), m_Pipeline(std::move(pipeline)), m_EmptyVertexArray(std::move(emptyVertexArray))
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
        Scope<VertexArray> emptyVertexArray = context.CreateVertexArray();
        if (pipeline == nullptr || emptyVertexArray == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create a tone mapper: its pipeline cannot be created");
            return nullptr;
        }
        return Scope<ToneMapper>(new ToneMapper(context, std::move(shader), std::move(pipeline), std::move(emptyVertexArray)));
    }

    void ToneMapper::Apply(const Framebuffer& source, Framebuffer* target, float exposure, Tonemapping tonemapping)
    {
        if (target != nullptr)
        {
            target->Bind();
        }
        else
        {
            m_Context->BindWindow();
        }

        m_Shader->SetFloat("u_Exposure", exposure);
        m_Shader->SetInt("u_Tonemapping", static_cast<int>(tonemapping));
        m_Pipeline->Bind();
        source.BindColorTexture(0, 0);
        m_Context->Draw(*m_EmptyVertexArray, 3);

        if (target != nullptr)
        {
            target->Unbind();
            m_Context->BindWindow();
        }
    }
}
