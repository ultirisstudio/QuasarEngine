#pragma once

#include "CollisionShape.h"
#include <glm/glm.hpp>

namespace QuasarEngine {

    class BoxShape : public CollisionShape {
    public:
        BoxShape(const glm::vec3& halfExtents);

        CollisionShapeType GetType() const override { return CollisionShapeType::Box; }

        glm::vec3 GetLocalBoundsMin() const override;
        glm::vec3 GetLocalBoundsMax() const override;

        float GetVolume() const override;

        float ComputeVolume() const override;
        glm::mat3 ComputeLocalInertiaTensor(float mass) const override;

        const glm::vec3& GetHalfExtents() const { return mHalfExtents; }
        AABB ComputeAABB(const glm::vec3& position, const glm::quat& orientation) const override;

    private:
        glm::vec3 mHalfExtents;
    };

}