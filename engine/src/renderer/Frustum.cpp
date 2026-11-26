#include <quasar/renderer/Frustum.h>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <algorithm>

namespace Quasar
{
    Frustum Frustum::FromViewProjection(const glm::mat4& viewProjection)
    {
        const auto row = [&viewProjection](int index)
        { return glm::vec4(viewProjection[0][index], viewProjection[1][index], viewProjection[2][index], viewProjection[3][index]); };
        const glm::vec4 x = row(0);
        const glm::vec4 y = row(1);
        const glm::vec4 z = row(2);
        const glm::vec4 w = row(3);

        Frustum frustum;
        frustum.m_Planes = {w + x, w - x, w + y, w - y, z, w - z};
        return frustum;
    }

    bool Frustum::Intersects(const BoundingBox& box) const
    {
        return std::ranges::all_of(m_Planes,
                                   [&box](const glm::vec4& plane)
                                   {
                                       const glm::vec3 normal(plane);
                                       const glm::vec3 farthest(normal.x >= 0.0f ? box.Max.x : box.Min.x, normal.y >= 0.0f ? box.Max.y : box.Min.y,
                                                                normal.z >= 0.0f ? box.Max.z : box.Min.z);
                                       return glm::dot(normal, farthest) + plane.w >= 0.0f;
                                   });
    }
}
