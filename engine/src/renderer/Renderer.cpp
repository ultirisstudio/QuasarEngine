#include <quasar/renderer/Renderer.h>

#include <quasar/log/Log.h>
#include <quasar/renderer/Color.h>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

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

        struct PointLightData
        {
            glm::vec4 PositionRange{0.0f};
            glm::vec4 Color{0.0f};
        };

        struct SpotLightData
        {
            glm::vec4 PositionRange{0.0f};
            glm::vec4 DirectionOuter{0.0f};
            glm::vec4 ColorInner{0.0f};
        };

        struct LightsData
        {
            glm::vec3 DirectionalDirection{0.0f, -1.0f, 0.0f};
            float DirectionalAmbient = 0.0f;
            glm::vec3 DirectionalColor{0.0f};
            std::int32_t PointLightCount = 0;
            std::int32_t SpotLightCount = 0;
            std::array<std::int32_t, 3> Padding{};
            std::array<PointLightData, Renderer::MaxPointLights> PointLights{};
            std::array<SpotLightData, Renderer::MaxSpotLights> SpotLights{};
        };

        static_assert(sizeof(LightsData) == 48 + (32 * Renderer::MaxPointLights) + (48 * Renderer::MaxSpotLights), "The Lights block follows std140");

        glm::vec3 Normalize(const glm::vec3& direction, const glm::vec3& fallback)
        {
            const float length = glm::length(direction);
            return length > 0.0f ? direction / length : fallback;
        }

        glm::vec3 ToRadiance(const glm::vec3& color, float intensity)
        {
            return SrgbToLinear(color) * intensity;
        }

        void SetDirectional(LightsData& data, const std::optional<DirectionalLight>& light)
        {
            if (light)
            {
                data.DirectionalDirection = Normalize(light->Direction, glm::vec3(0.0f, -1.0f, 0.0f));
                data.DirectionalAmbient = light->Ambient;
                data.DirectionalColor = ToRadiance(light->Color, light->Intensity);
            }
        }

        void SetPoints(LightsData& data, const std::vector<PointLight>& lights)
        {
            const std::size_t count = std::min(lights.size(), data.PointLights.size());
            for (std::size_t index = 0; index < count; ++index)
            {
                const PointLight& light = lights[index];
                data.PointLights.at(index) = {.PositionRange = glm::vec4(light.Position, light.Range),
                                              .Color = glm::vec4(ToRadiance(light.Color, light.Intensity), 0.0f)};
            }
            data.PointLightCount = static_cast<std::int32_t>(count);
        }

        void SetSpots(LightsData& data, const std::vector<SpotLight>& lights)
        {
            const std::size_t count = std::min(lights.size(), data.SpotLights.size());
            for (std::size_t index = 0; index < count; ++index)
            {
                const SpotLight& light = lights[index];
                const float outer = std::cos(light.OuterAngle);
                const float inner = std::max(std::cos(light.InnerAngle), outer + 1e-4f);
                data.SpotLights.at(index) = {.PositionRange = glm::vec4(light.Position, light.Range),
                                             .DirectionOuter = glm::vec4(Normalize(light.Direction, glm::vec3(0.0f, 0.0f, -1.0f)), outer),
                                             .ColorInner = glm::vec4(ToRadiance(light.Color, light.Intensity), inner)};
            }
            data.SpotLightCount = static_cast<std::int32_t>(count);
        }

        LightsData ToLightsData(const SceneLights& lights)
        {
            LightsData data;
            SetDirectional(data, lights.Directional);
            SetPoints(data, lights.Points);
            SetSpots(data, lights.Spots);
            return data;
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
        Scope<UniformBuffer> light = context.CreateUniformBuffer(sizeof(LightsData), LightBinding);
        if (camera == nullptr || object == nullptr || light == nullptr)
        {
            return nullptr;
        }
        return Scope<Renderer>(new Renderer(context, std::move(camera), std::move(object), std::move(light)));
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
        m_LightBuffer->Set(ToLightsData(lights));
        const bool tooMany = lights.Points.size() > MaxPointLights || lights.Spots.size() > MaxSpotLights;
        if (tooMany && !m_ReportedTooManyLights)
        {
            QUASAR_ENGINE_WARN("The scene has {} point lights and {} spot lights, but only the first {} and {} are used", lights.Points.size(),
                               lights.Spots.size(), MaxPointLights, MaxSpotLights);
        }
        m_ReportedTooManyLights = tooMany;
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
