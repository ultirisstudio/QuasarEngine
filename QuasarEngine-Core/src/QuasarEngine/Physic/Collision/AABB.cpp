#include "qepch.h"
#include "AABB.h"

namespace QuasarEngine {

    AABB::AABB(const glm::vec3& min, const glm::vec3& max)
        : min(min), max(max) {
    }

    bool AABB::Overlaps(const AABB& other) const {
        // Version SIMD-friendly
        return !(other.max.x < min.x || other.min.x > max.x ||
            other.max.y < min.y || other.min.y > max.y ||
            other.max.z < min.z || other.min.z > max.z);
    }

    AABB AABB::Merge(const AABB& other) const {
        return AABB(glm::min(min, other.min), glm::max(max, other.max));
    }

    glm::vec3 AABB::GetCenter() const {
        return 0.5f * (min + max);
    }

    // Vérifie si un point est contenu dans l'AABB
    bool AABB::Contains(const glm::vec3& point) const {
        return (point.x >= min.x && point.x <= max.x) &&
            (point.y >= min.y && point.y <= max.y) &&
            (point.z >= min.z && point.z <= max.z);
    }

    // Étend l'AABB pour inclure un point
    void AABB::Expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }

    glm::vec3 AABB::GetExtents() const {
        return 0.5f * (max - min);
    }

    float AABB::GetSurfaceArea() const {
        glm::vec3 extents = max - min;
        return 2.0f * (extents.x * extents.y + extents.x * extents.z + extents.y * extents.z);
    }

    bool AABB::RayIntersect(const glm::vec3& origin, const glm::vec3& direction, float& tmin, float& tmax) const {
        tmin = 0.0f;
        tmax = FLT_MAX;

        const glm::vec3 invDir = 1.0f / direction; // Évite la division répétée

        for (int i = 0; i < 3; ++i) {
            if (std::abs(direction[i]) < 1e-6f) {
                if (origin[i] < min[i] || origin[i] > max[i])
                    return false;
            }
            else {
                float t1 = (min[i] - origin[i]) * invDir[i];
                float t2 = (max[i] - origin[i]) * invDir[i];
                if (t1 > t2) std::swap(t1, t2);
                tmin = glm::max(tmin, t1);
                tmax = glm::min(tmax, t2);
                if (tmin > tmax) return false;
            }
        }

        return true;
    }

}
