#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>

namespace QuasarEngine {

    class Collider;

    enum class RigidBodyType {
        Static,
        Kinematic,
        Dynamic
    };

    class RigidBody {
    public:
        RigidBody(const glm::vec3& position, const glm::quat& orientation, float mass);
        RigidBody();

        void ApplyForce(const glm::vec3& force);
        void ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& point);
        void Integrate(float dt);

        void AddCollider(Collider* collider);

        bool IsDynamic() const { return type == RigidBodyType::Dynamic; }

        // Getters / Setters
        glm::vec3 GetPosition() const { return position; }
        glm::quat GetOrientation() const { return orientation; }

        void SetLinearVelocity(const glm::vec3& v) { linearVelocity = v; }
        const glm::vec3& GetLinearVelocity() const { return linearVelocity; }

        void SetAngularVelocity(const glm::vec3& w) { angularVelocity = w; }
        const glm::vec3& GetAngularVelocity() const { return angularVelocity; }

        void SetEnableGravity(bool value);
        bool UsesGravity() const;

        // Type & masse
        RigidBodyType type;
        float mass;
        float inverseMass;

        // Accumulateurs de forces
        glm::vec3 forceAccumulator;
        glm::vec3 torqueAccumulator;

        // État dynamique
        glm::vec3 linearVelocity;
        glm::vec3 angularVelocity;
        glm::vec3 position;
        glm::quat orientation;

        // Inertie
        glm::mat3 inertiaTensorLocal;
        glm::mat3 inverseInertiaTensorWorld;

        float linearDamping = 0.01f;
        float angularDamping = 0.05f;

        // Gravity
        bool enableGravity = true;

        void UpdateInertiaTensor();

        std::vector<Collider*> colliders;

    private:
        void ClearAccumulators();
    };

}