#include "qepch.h"
#include "SphereShape.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace QuasarEngine {

    SphereShape::SphereShape(float radius)
        : mRadius(radius) {
    }

    glm::vec3 SphereShape::GetLocalBoundsMin() const {
        return glm::vec3(-mRadius);
    }

    glm::vec3 SphereShape::GetLocalBoundsMax() const {
        return glm::vec3(mRadius);
    }

    float SphereShape::GetVolume() const {
        return ComputeVolume();
    }

    float SphereShape::ComputeVolume() const {
        return (4.0f / 3.0f) * glm::pi<float>() * mRadius * mRadius * mRadius;
    }

    glm::mat3 SphereShape::ComputeLocalInertiaTensor(float mass) const {
        float coeff = (2.0f / 5.0f) * mass * mRadius * mRadius;
        return glm::mat3(coeff);
    }

    AABB SphereShape::ComputeAABB(const glm::vec3& position, const glm::quat& orientation) const {
        float r = GetRadius();
        glm::vec3 radiusVec(r, r, r);
        return AABB(position - radiusVec, position + radiusVec);
    }

}