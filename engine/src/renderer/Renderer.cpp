#include <quasar/renderer/Renderer.h>

#include <quasar/log/Log.h>

#include <glm/vec4.hpp>

#include <algorithm>
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

    Renderer::Renderer(GraphicsContext& context, Scope<UniformBuffer> camera, Scope<UniformBuffer> object)
        : m_Context(&context), m_CameraBuffer(std::move(camera)), m_ObjectBuffer(std::move(object))
    {
    }

    Scope<Renderer> Renderer::Create(GraphicsContext& context)
    {
        Scope<UniformBuffer> camera = context.CreateUniformBuffer(sizeof(CameraData), CameraBinding);
        Scope<UniformBuffer> object = context.CreateUniformBuffer(sizeof(glm::mat4), ObjectBinding);
        if (camera == nullptr || object == nullptr)
        {
            return nullptr;
        }
        return Scope<Renderer>(new Renderer(context, std::move(camera), std::move(object)));
    }

    void Renderer::BeginScene(const Camera& camera, Framebuffer* target)
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
        const std::uint32_t binding = material.GetSpecification().UniformBinding;
        if (!material.GetSpecification().Uniforms.GetMembers().empty() && (binding == CameraBinding || binding == ObjectBinding))
        {
            if (std::ranges::find(m_ReportedMaterials, &material) == m_ReportedMaterials.end())
            {
                m_ReportedMaterials.push_back(&material);
                QUASAR_ENGINE_ERROR("Cannot draw with a material on the binding {}, which the renderer uses", binding);
            }
            return;
        }

        m_ObjectBuffer->Set(transform);
        material.Bind();
        m_CameraBuffer->Bind();
        m_ObjectBuffer->Bind();
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
