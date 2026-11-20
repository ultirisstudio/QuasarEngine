#include <quasar/renderer/Shadows.h>

#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace Quasar
{
    namespace
    {
        Camera ClipToDistance(Camera camera, float distance)
        {
            if (camera.GetProjectionType() == ProjectionType::Perspective)
            {
                PerspectiveProjection projection = camera.GetPerspective();
                projection.Far = std::max(std::min(projection.Far, distance), projection.Near * 2.0f);
                camera.SetPerspective(projection);
            }
            else
            {
                OrthographicProjection projection = camera.GetOrthographic();
                projection.Far = std::min(projection.Far, projection.Near + distance);
                camera.SetOrthographic(projection);
            }
            return camera;
        }

        std::array<glm::vec3, 8> GetCorners(const Camera& camera)
        {
            const glm::mat4 toWorld = glm::inverse(camera.GetViewProjection());
            std::array<glm::vec3, 8> corners{};
            for (std::size_t index = 0; index < corners.size(); ++index)
            {
                const glm::vec4 corner =
                    toWorld * glm::vec4((index & 1U) != 0 ? 1.0f : -1.0f, (index & 2U) != 0 ? 1.0f : -1.0f, (index & 4U) != 0 ? 1.0f : 0.0f, 1.0f);
                corners.at(index) = glm::vec3(corner) / corner.w;
            }
            return corners;
        }
    }

    ShadowCamera ComputeShadowCamera(const Camera& camera, const glm::vec3& direction, const ShadowSettings& settings)
    {
        const std::uint32_t resolution = std::max(settings.Resolution, 1U);
        const float distance = std::max(settings.Distance, 0.01f);

        const std::array<glm::vec3, 8> corners = GetCorners(ClipToDistance(camera, distance));
        glm::vec3 center(0.0f);
        for (const glm::vec3& corner : corners)
        {
            center += corner / static_cast<float>(corners.size());
        }
        float radius = 0.0f;
        for (const glm::vec3& corner : corners)
        {
            radius = std::max(radius, glm::distance(corner, center));
        }
        const float texel = 2.0f * radius / static_cast<float>(resolution);

        const float length = glm::length(direction);
        const glm::vec3 forward = length > 0.0f ? direction / length : glm::vec3(0.0f, -1.0f, 0.0f);
        const glm::vec3 up = std::abs(forward.y) > 0.99f ? glm::vec3(0.0f, 0.0f, -1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        const float backward = radius + distance;
        ShadowCamera shadow{.View = Camera(), .Resolution = resolution, .TexelSize = texel};
        shadow.View.SetViewportSize(resolution, resolution);
        shadow.View.SetOrthographic({.Height = 2.0f * radius, .Near = 0.0f, .Far = backward + radius});
        shadow.View.SetPosition(center - (forward * backward));
        shadow.View.LookAt(center, up);

        glm::vec3 local = glm::conjugate(shadow.View.GetOrientation()) * center;
        local.x = std::floor(local.x / texel) * texel;
        local.y = std::floor(local.y / texel) * texel;
        shadow.View.SetPosition((shadow.View.GetOrientation() * local) - (forward * backward));
        return shadow;
    }
}
