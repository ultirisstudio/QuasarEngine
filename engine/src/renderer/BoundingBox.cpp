#include <quasar/renderer/BoundingBox.h>

#include <glm/common.hpp>
#include <glm/vec4.hpp>

namespace Quasar
{
    BoundingBox BoundingBox::FromPoints(std::span<const glm::vec3> points)
    {
        if (points.empty())
        {
            return {};
        }
        BoundingBox box{.Min = points.front(), .Max = points.front()};
        for (const glm::vec3& point : points)
        {
            box.Min = glm::min(box.Min, point);
            box.Max = glm::max(box.Max, point);
        }
        return box;
    }

    BoundingBox BoundingBox::Transformed(const glm::mat4& transform) const
    {
        const glm::vec3 center = (Min + Max) * 0.5f;
        const glm::vec3 extent = (Max - Min) * 0.5f;
        const glm::vec3 placed(transform * glm::vec4(center, 1.0f));
        const glm::vec3 reach =
            (glm::abs(glm::vec3(transform[0])) * extent.x) + (glm::abs(glm::vec3(transform[1])) * extent.y) + (glm::abs(glm::vec3(transform[2])) * extent.z);
        return {.Min = placed - reach, .Max = placed + reach};
    }
}
