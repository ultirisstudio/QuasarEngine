#include <quasar/renderer/Renderer.h>

#include <quasar/log/Log.h>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
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

        struct LightData
        {
            glm::vec3 Direction{0.0f, -1.0f, 0.0f};
            float Ambient = 0.0f;
            glm::vec3 Color{0.0f};
            float Padding = 0.0f;
        };

        LightData ToLightData(const DirectionalLight& light)
        {
            const float length = glm::length(light.Direction);
            return {.Direction = length > 0.0f ? light.Direction / length : glm::vec3(0.0f, -1.0f, 0.0f),
                    .Ambient = light.Ambient,
                    .Color = light.Color * light.Intensity};
        }
    }

    Renderer::Renderer(GraphicsContext& context, Scope<UniformBuffer> camera, Scope<UniformBuffer> object, Scope<UniformBuffer> light)
        : m_Context(&context), m_CameraBuffer(std::move(camera)), m_ObjectBuffer(std::move(object)), m_LightBuffer(std::move(light))
    {
    }

    Scope<Renderer> Renderer::Create(GraphicsContext& context)
    {
        Scope<UniformBuffer> camera = context.CreateUniformBuffer(sizeof(CameraData), CameraBinding);
        Scope<UniformBuffer> object = context.CreateUniformBuffer(sizeof(glm::mat4), ObjectBinding);
        Scope<UniformBuffer> light = context.CreateUniformBuffer(sizeof(LightData), LightBinding);
        if (camera == nullptr || object == nullptr || light == nullptr)
        {
            return nullptr;
        }
        return Scope<Renderer>(new Renderer(context, std::move(camera), std::move(object), std::move(light)));
    }

    void Renderer::BeginScene(const Camera& camera, Framebuffer* target, const DirectionalLight& light)
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
        m_LightBuffer->Set(ToLightData(light));
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
        if (!material.GetSpecification().Uniforms.GetMembers().empty() && (binding == CameraBinding || binding == ObjectBinding || binding == LightBinding))
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
        m_LightBuffer->Bind();
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
