#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <QuasarEngine/Physic/Collision/AABB.h>

namespace QuasarEngine {

    enum class CollisionShapeType {
        Box,
        Sphere,
        // Capsule, Mesh, etc. à ajouter plus tard
    };

    class CollisionShape {
    public:
        CollisionShape() = default;
        virtual ~CollisionShape() = default;

        virtual CollisionShapeType GetType() const = 0;

        // Retourne les AABB en espace local (centré à l'origine, pas transformé)
        virtual glm::vec3 GetLocalBoundsMin() const = 0;
        virtual glm::vec3 GetLocalBoundsMax() const = 0;

        // Pour debug draw, collision test, etc.
        virtual float GetVolume() const = 0;

        virtual AABB ComputeAABB(const glm::vec3& position, const glm::quat& orientation) const = 0;

        virtual float ComputeVolume() const = 0;
        virtual glm::mat3 ComputeLocalInertiaTensor(float mass) const = 0;
    };

}