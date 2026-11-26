#include <quasar/renderer/Renderer.h>

#include <renderer/LightsBuffer.h>

#include <quasar/log/Log.h>
#include <quasar/renderer/ShaderBindings.h>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstdint>
#include <optional>
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

        struct ObjectData
        {
            glm::mat4 Model{1.0f};
            std::array<glm::vec4, 3> NormalMatrix{};
        };
        static_assert(sizeof(ObjectData) == 112, "The Object block follows std140");

        ObjectData ToObjectData(const glm::mat4& transform)
        {
            const glm::mat3 normal = glm::inverseTranspose(glm::mat3(transform));
            return {.Model = transform, .NormalMatrix = {glm::vec4(normal[0], 0.0f), glm::vec4(normal[1], 0.0f), glm::vec4(normal[2], 0.0f)}};
        }
    }

    Renderer::Renderer(GraphicsContext& context, Scope<FrameUniformBuffer> camera, Scope<FrameUniformBuffer> object, Scope<LightsBuffer> lights)
        : m_Context(&context), m_CameraBuffer(std::move(camera)), m_ObjectBuffer(std::move(object)), m_LightsBuffer(std::move(lights))
    {
    }

    Renderer::~Renderer() = default;

    Scope<Renderer> Renderer::Create(GraphicsContext& context)
    {
        Scope<FrameUniformBuffer> camera = context.CreateFrameUniformBuffer(sizeof(CameraData), ShaderBindings::Camera);
        Scope<FrameUniformBuffer> object = context.CreateFrameUniformBuffer(sizeof(ObjectData), ShaderBindings::Object);
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
        const std::optional<std::uint32_t> cameraPart = m_CameraBuffer->Add(CameraData{.ViewProjection = camera.GetViewProjection(),
                                                                                       .View = camera.GetView(),
                                                                                       .Projection = camera.GetProjection(),
                                                                                       .Position = glm::vec4(camera.GetPosition(), 1.0f)});
        if (cameraPart)
        {
            m_CameraBuffer->Bind(*cameraPart);
        }
        m_LightsBuffer->Bind(lights);
        m_BoundPipeline = nullptr;
        m_BoundMaterial = nullptr;
        if (lights.Shadow.Map != nullptr)
        {
            lights.Shadow.Map->BindDepthTexture(ShaderBindings::ShadowMap);
        }
        if (lights.AmbientOcclusion.Map != nullptr)
        {
            lights.AmbientOcclusion.Map->BindColorTexture(0, ShaderBindings::AmbientOcclusionMap);
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

    std::optional<PlacedObject> Renderer::PlaceObject(const glm::mat4& transform)
    {
        const std::optional<std::uint32_t> part = m_ObjectBuffer->Add(ToObjectData(transform));
        if (!part)
        {
            return std::nullopt;
        }
        return PlacedObject{.Part = *part, .Frame = m_Context->GetFrameNumber()};
    }

    void Renderer::Submit(const Mesh& mesh, Material& material, PlacedObject object)
    {
        if (!CanDraw(object))
        {
            return;
        }
        BindPipeline(*material.GetSpecification().Pipeline);
        if (&material != m_BoundMaterial || material.HasChanged())
        {
            material.BindResources();
            m_BoundMaterial = &material;
        }
        Draw(mesh, object);
    }

    void Renderer::Submit(const Mesh& mesh, Material& material, const glm::mat4& transform)
    {
        if (const std::optional<PlacedObject> object = PlaceObject(transform))
        {
            Submit(mesh, material, *object);
        }
    }

    void Renderer::Submit(const Mesh& mesh, Pipeline& pipeline, PlacedObject object)
    {
        if (!CanDraw(object))
        {
            return;
        }
        BindPipeline(pipeline);
        Draw(mesh, object);
    }

    void Renderer::Submit(const Mesh& mesh, Pipeline& pipeline, const glm::mat4& transform)
    {
        if (const std::optional<PlacedObject> object = PlaceObject(transform))
        {
            Submit(mesh, pipeline, *object);
        }
    }

    bool Renderer::CanDraw(PlacedObject object) const
    {
        if (!m_InScene)
        {
            QUASAR_ENGINE_ERROR("Cannot draw a mesh outside of a scene");
            return false;
        }
        if (object.Frame != m_Context->GetFrameNumber())
        {
            QUASAR_ENGINE_ERROR("Cannot draw a mesh placed in another frame");
            return false;
        }
        return true;
    }

    void Renderer::BindPipeline(Pipeline& pipeline)
    {
        if (&pipeline != m_BoundPipeline)
        {
            pipeline.Bind();
            m_BoundPipeline = &pipeline;
        }
    }

    void Renderer::Draw(const Mesh& mesh, PlacedObject object)
    {
        m_ObjectBuffer->Bind(object.Part);
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
