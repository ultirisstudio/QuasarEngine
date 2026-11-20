#include <quasar/renderer/Renderer.h>

#include <renderer/LightsBuffer.h>

#include <quasar/log/Log.h>
#include <quasar/renderer/ShaderBindings.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <utility>

namespace Quasar
{
    namespace
    {
        struct CameraData
        {
            glm::mat4 ViewProjection{1.0f};
            glm::mat4 View{1.0f};
            glm::mat4 Projection{1.0f};
            glm::vec4 Position{0.0f};
        };
    }

    Renderer::Renderer(GraphicsContext& context, Scope<UniformBuffer> camera, Scope<UniformBuffer> object, Scope<LightsBuffer> lights)
        : m_Context(&context), m_CameraBuffer(std::move(camera)), m_ObjectBuffer(std::move(object)), m_LightsBuffer(std::move(lights))
    {
    }

    Renderer::~Renderer() = default;

    Scope<Renderer> Renderer::Create(GraphicsContext& context)
    {
        Scope<UniformBuffer> camera = context.CreateUniformBuffer(sizeof(CameraData), ShaderBindings::Camera);
        Scope<UniformBuffer> object = context.CreateUniformBuffer(sizeof(glm::mat4), ShaderBindings::Object);
        Scope<LightsBuffer> lights = LightsBuffer::Create(context);
        if (camera == nullptr || object == nullptr || lights == nullptr)
        {
            return nullptr;
        }
        return Scope<Renderer>(new Renderer(context, std::move(camera), std::move(object), std::move(lights)));
    }

    void Renderer::BeginScene(const Camera& camera, Framebuffer* target, const SceneLights& lights)
    {
        if (m_InScene)
        {
            QUASAR_ENGINE_ERROR("Cannot begin a scene before the previous one ends");
            return;
        }

        m_InScene = true;
        m_Target = target;
        m_CameraBuffer->Set(CameraData{.ViewProjection = camera.GetViewProjection(),
                                       .View = camera.GetView(),
                                       .Projection = camera.GetProjection(),
                                       .Position = glm::vec4(camera.GetPosition(), 1.0f)});
        m_LightsBuffer->Set(lights);
        m_CameraBuffer->Bind();
        m_ObjectBuffer->Bind();
        m_LightsBuffer->Bind();
        m_BoundPipeline = nullptr;
        m_BoundMaterial = nullptr;
        if (lights.Shadow.Map != nullptr)
        {
            lights.Shadow.Map->BindDepthTexture(ShaderBindings::ShadowMap);
        }
        if (m_Target != nullptr)
        {
            m_Target->Bind();
        }
        else
        {
            m_Context->BindWindow();
        }
    }

    void Renderer::Submit(const Mesh& mesh, Material& material, const glm::mat4& transform)
    {
        if (!m_InScene)
        {
            QUASAR_ENGINE_ERROR("Cannot draw a mesh outside of a scene");
            return;
        }
        BindPipeline(*material.GetSpecification().Pipeline);
        if (&material != m_BoundMaterial || material.HasChanged())
        {
            material.BindResources();
            m_BoundMaterial = &material;
        }
        Draw(mesh, transform);
    }

    void Renderer::Submit(const Mesh& mesh, Pipeline& pipeline, const glm::mat4& transform)
    {
        if (!m_InScene)
        {
            QUASAR_ENGINE_ERROR("Cannot draw a mesh outside of a scene");
            return;
        }
        BindPipeline(pipeline);
        Draw(mesh, transform);
    }

    void Renderer::BindPipeline(Pipeline& pipeline)
    {
        if (&pipeline != m_BoundPipeline)
        {
            pipeline.Bind();
            m_BoundPipeline = &pipeline;
        }
    }

    void Renderer::Draw(const Mesh& mesh, const glm::mat4& transform)
    {
        m_ObjectBuffer->Set(transform);
        m_Context->DrawIndexed(mesh.GetVertexArray());
    }

    void Renderer::EndScene()
    {
        if (!m_InScene)
        {
            QUASAR_ENGINE_ERROR("Cannot end a scene that did not begin");
            return;
        }

        if (m_Target != nullptr)
        {
            m_Target->Unbind();
            m_Context->BindWindow();
        }
        m_Target = nullptr;
        m_InScene = false;
    }
}
