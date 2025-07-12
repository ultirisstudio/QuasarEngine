#pragma once

#include <glm/glm.hpp>
#include <algorithm>

namespace QuasarEngine {

    class AABB {
    public:
        glm::vec3 min;
        glm::vec3 max;

        AABB() = default;
        AABB(const glm::vec3& min, const glm::vec3& max);

        bool RayIntersect(const glm::vec3& origin, const glm::vec3& direction, float& tmin, float& tmax) const;


        bool Overlaps(const AABB& other) const;
        AABB Merge(const AABB& other) const;
        glm::vec3 GetCenter() const;
        glm::vec3 GetExtents() const;
        float GetSurfaceArea() const;
        bool AABB::Contains(const glm::vec3& point) const;
        void AABB::Expand(const glm::vec3& point);

        bool Contains(const AABB& other) const {
            return (other.min.x >= min.x && other.max.x <= max.x) &&
                (other.min.y >= min.y && other.max.y <= max.y) &&
                (other.min.z >= min.z && other.max.z <= max.z);
        }
    };

}