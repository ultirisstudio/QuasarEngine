#include "qepch.h"
#include "ProxyShape.h"
#include "QuasarEngine/Physic/RigidBody.h"
#include "QuasarEngine/Physic/Collision/Collider.h"
#include "CollisionShape.h"
#include <memory>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace QuasarEngine {

    ProxyShape::ProxyShape(RigidBody* body, Collider* collider, std::unique_ptr<CollisionShape> shape)
        : rigidBody(body), collider(collider), collisionShape(std::move(shape))
    {

    }

    glm::vec3 ProxyShape::GetWorldPosition() const {
        if (!rigidBody) return glm::vec3(0.0f);

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), rigidBody->position) * glm::toMat4(rigidBody->orientation);

        // Supposons que la forme est centrée à l’origine locale du rigidbody
        return glm::vec3(transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    glm::mat4 ProxyShape::GetWorldTransform() const {
        if (!rigidBody) return glm::mat4(1.0f);
        glm::mat4 translation = glm::translate(glm::mat4(1.0f), rigidBody->position);
        glm::mat4 rotation = glm::toMat4(rigidBody->orientation);
        return translation * rotation;
    }

}