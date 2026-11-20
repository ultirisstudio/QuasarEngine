#include <renderer/LightsBuffer.h>

#include <quasar/log/Log.h>
#include <quasar/renderer/Color.h>
#include <quasar/renderer/ShaderBindings.h>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace Quasar
{
    namespace
    {
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
            glm::mat4 ShadowViewProjection{1.0f};
            glm::vec4 Shadow{0.0f};
            std::array<PointLightData, MaxPointLights> PointLights{};
            std::array<SpotLightData, MaxSpotLights> SpotLights{};
        };

        static_assert(sizeof(LightsData) == 128 + (32 * MaxPointLights) + (48 * MaxSpotLights), "The Lights block follows std140");

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
            if (lights.Shadow.Map != nullptr)
            {
                data.ShadowViewProjection = lights.Shadow.ViewProjection;
                data.Shadow = {1.0f, lights.Shadow.DepthBias, lights.Shadow.NormalBias,
                               1.0f / static_cast<float>(std::max(lights.Shadow.Map->GetSpecification().Width, 1U))};
            }
            SetPoints(data, lights.Points);
            SetSpots(data, lights.Spots);
            return data;
        }
    }

    Scope<LightsBuffer> LightsBuffer::Create(GraphicsContext& context)
    {
        Scope<UniformBuffer> buffer = context.CreateUniformBuffer(sizeof(LightsData), ShaderBindings::Lights);
        return buffer != nullptr ? Scope<LightsBuffer>(new LightsBuffer(std::move(buffer))) : nullptr;
    }

    void LightsBuffer::Set(const SceneLights& lights)
    {
        const LightsData data = ToLightsData(lights);
        const std::span<const std::byte> bytes = std::as_bytes(std::span<const LightsData, 1>(&data, 1));
        const auto pointCount = static_cast<std::size_t>(data.PointLightCount);
        const auto spotCount = static_cast<std::size_t>(data.SpotLightCount);
        m_Buffer->SetData(bytes.first(offsetof(LightsData, PointLights) + (pointCount * sizeof(PointLightData))));
        if (spotCount > 0)
        {
            m_Buffer->SetData(bytes.subspan(offsetof(LightsData, SpotLights), spotCount * sizeof(SpotLightData)), offsetof(LightsData, SpotLights));
        }

        const bool tooMany = lights.Points.size() > MaxPointLights || lights.Spots.size() > MaxSpotLights;
        if (tooMany && !m_ReportedTooManyLights)
        {
            QUASAR_ENGINE_WARN("The scene has {} point lights and {} spot lights, but only the first {} and {} are used", lights.Points.size(),
                               lights.Spots.size(), MaxPointLights, MaxSpotLights);
        }
        m_ReportedTooManyLights = tooMany;
    }

    void LightsBuffer::Bind()
    {
        m_Buffer->Bind();
    }
}
