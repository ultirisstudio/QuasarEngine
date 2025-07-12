#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <QuasarEngine/Physic/Shape/CollisionShape.h>
#include <memory>

namespace QuasarEngine {

    class RigidBody;
    class Collider;

    class ProxyShape {
    public:
        ProxyShape(RigidBody* body,
            Collider* collider,
            std::unique_ptr<CollisionShape> shape);

        ~ProxyShape() = default;

        CollisionShape* GetCollisionShape() const { return collisionShape.get(); }
        RigidBody* GetBody() const { return rigidBody; }
        Collider* GetCollider() const { return collider; }

        float friction = 0.5f;
        float bounciness = 0.02f;

        void SetFriction(float f) { friction = glm::clamp(f, 0.0f, 10.0f); }
        void SetBounciness(float b) { bounciness = glm::clamp(b, 0.0f, 1.0f); }

        float GetFriction() const { return friction; }
        float GetBounciness() const { return bounciness; }

        Collider* GetParentCollider() const { return mParentCollider; }

        glm::vec3 GetWorldPosition() const;
        glm::mat4 GetWorldTransform() const;

    private:
        RigidBody* rigidBody;
        Collider* collider;
        std::unique_ptr<CollisionShape> collisionShape;

        Collider* mParentCollider = nullptr;
    };

}