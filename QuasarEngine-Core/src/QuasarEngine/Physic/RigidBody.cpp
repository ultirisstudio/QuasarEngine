#include "qepch.h"
#include "RigidBody.h"
#include "QuasarEngine/Physic/Shape/ProxyShape.h"
#include "QuasarEngine/Physic/Collision/Collider.h"
#include "QuasarEngine/Physic/Shape/CollisionShape.h"
#include <memory>

namespace QuasarEngine {

    RigidBody::RigidBody(const glm::vec3& pos, const glm::quat& ori, float m)
        : position(pos), orientation(ori), mass(m), type(RigidBodyType::Dynamic)
    {
        inverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
        linearVelocity = glm::vec3(0.0f);
        angularVelocity = glm::vec3(0.0f);
        forceAccumulator = glm::vec3(0.0f);
        torqueAccumulator = glm::vec3(0.0f);

        if (glm::length(orientation) < 0.0001f) {
            orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        inertiaTensorLocal = glm::mat3(1.0f);  // à remplacer plus tard
        inverseInertiaTensorWorld = glm::inverse(inertiaTensorLocal);
    }

    RigidBody::RigidBody()
        : position(0.0f), orientation(glm::quat(1.0f, 0, 0, 0)), mass(1.0f), type(RigidBodyType::Dynamic)
    {
        inverseMass = 1.0f / mass;
        linearVelocity = glm::vec3(0.0f);
        angularVelocity = glm::vec3(0.0f);
        forceAccumulator = glm::vec3(0.0f);
        torqueAccumulator = glm::vec3(0.0f);
        inertiaTensorLocal = glm::mat3(1.0f);
        inverseInertiaTensorWorld = glm::inverse(inertiaTensorLocal);
    }

    void RigidBody::ApplyForce(const glm::vec3& force) {
        forceAccumulator += force;
    }

    void RigidBody::ApplyForceAtPoint(const glm::vec3& force, const glm::vec3& point) {
        forceAccumulator += force;
        glm::vec3 r = point - position;
        torqueAccumulator += glm::cross(r, force);
    }

    void RigidBody::Integrate(float dt) {
        if (type == RigidBodyType::Static || mass <= 0.0f) return;

        // LINEAR MOTION
        glm::vec3 acceleration = forceAccumulator * inverseMass;
        linearVelocity += acceleration * dt;
        position += linearVelocity * dt;

        // ANGULAR MOTION
        glm::vec3 angularAcceleration = inverseInertiaTensorWorld * torqueAccumulator;
        angularVelocity += angularAcceleration * dt;

        // CORRECT quaternion integration
        glm::quat angularVelQuat(0.0f, angularVelocity.x, angularVelocity.y, angularVelocity.z);
        glm::quat deltaOrientation = 0.5f * angularVelQuat * orientation;
        orientation += deltaOrientation * dt;
        orientation = glm::normalize(orientation);

        // Update inertia tensor (if orientation changed)
        UpdateInertiaTensor();

        // Clear forces for next frame
        ClearAccumulators();
    }

    void RigidBody::UpdateInertiaTensor() {
        glm::mat3 rot = glm::mat3_cast(orientation);
        inverseInertiaTensorWorld = rot * glm::inverse(inertiaTensorLocal) * glm::transpose(rot);
    }

    void RigidBody::ClearAccumulators() {
        forceAccumulator = glm::vec3(0.0f);
        torqueAccumulator = glm::vec3(0.0f);
    }

    void RigidBody::AddCollider(Collider* collider)
    {
        colliders.push_back(collider);
    }

    void RigidBody::SetEnableGravity(bool value) {
        enableGravity = value;
    }

    bool RigidBody::UsesGravity() const {
        return enableGravity && IsDynamic();
    }

}