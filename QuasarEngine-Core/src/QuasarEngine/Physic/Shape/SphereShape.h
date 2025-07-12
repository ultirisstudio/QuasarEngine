#pragma once

#include <QuasarEngine/Physic/Shape/CollisionShape.h>
#include <glm/glm.hpp>

namespace QuasarEngine {

    class SphereShape : public CollisionShape {
    public:
        SphereShape(float radius);
        ~SphereShape() override = default;

        CollisionShapeType GetType() const override { return CollisionShapeType::Sphere; }

        float GetRadius() const { return mRadius; }
        void SetRadius(float radius) { mRadius = radius; }

        glm::vec3 GetLocalBoundsMin() const override;
        glm::vec3 GetLocalBoundsMax() const override;

        float GetVolume() const override;
        float ComputeVolume() const override;
        glm::mat3 ComputeLocalInertiaTensor(float mass) const override;

        AABB ComputeAABB(const glm::vec3& position, const glm::quat& orientation) const override;

    private:
        float mRadius;
    };

}