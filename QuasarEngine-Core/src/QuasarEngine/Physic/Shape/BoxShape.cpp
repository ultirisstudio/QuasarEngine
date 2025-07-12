#include "qepch.h"
#include "BoxShape.h"

namespace QuasarEngine {

    BoxShape::BoxShape(const glm::vec3& halfExtents)
        : mHalfExtents(halfExtents)
    {
    }

    glm::vec3 BoxShape::GetLocalBoundsMin() const {
        return -mHalfExtents;
    }

    glm::vec3 BoxShape::GetLocalBoundsMax() const {
        return mHalfExtents;
    }

    float BoxShape::GetVolume() const {
        glm::vec3 fullSize = mHalfExtents * 2.0f;
        return fullSize.x * fullSize.y * fullSize.z;
    }

    float BoxShape::ComputeVolume() const {
        return GetVolume();
    }

    AABB BoxShape::ComputeAABB(const glm::vec3& position, const glm::quat& orientation) const {
        glm::mat3 rotationMatrix = glm::mat3_cast(orientation);
        glm::vec3 extents = GetHalfExtents();

        glm::vec3 worldExtents = glm::abs(rotationMatrix[0]) * extents.x +
            glm::abs(rotationMatrix[1]) * extents.y +
            glm::abs(rotationMatrix[2]) * extents.z;

        return AABB(position - worldExtents, position + worldExtents);
    }

    glm::mat3 BoxShape::ComputeLocalInertiaTensor(float mass) const {
        glm::vec3 size = mHalfExtents * 2.0f;
        float x2 = size.x * size.x;
        float y2 = size.y * size.y;
        float z2 = size.z * size.z;

        float factor = (1.0f / 12.0f) * mass;
        return glm::mat3{
            factor * (y2 + z2), 0, 0,
            0, factor * (x2 + z2), 0,
            0, 0, factor * (x2 + y2)
        };
    }

}