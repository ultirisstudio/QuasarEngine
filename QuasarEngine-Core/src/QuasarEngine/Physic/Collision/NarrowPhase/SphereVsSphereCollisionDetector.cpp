#include "qepch.h"
#include "SphereVsSphereCollisionDetector.h"

#include <QuasarEngine/Physic/Shape/SphereShape.h>
#include <QuasarEngine/Physic/Collision/Collider.h>
#include <QuasarEngine/Physic/Collision/ContactManifold.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/RigidBody.h>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp> // for distance2

namespace QuasarEngine {

    bool SphereVsSphereCollisionDetector::TestCollision(Collider* colliderA, Collider* colliderB, ContactManifold& manifold) {
        if (!colliderA || !colliderB)
            return false;

        ProxyShape* shapeA = colliderA->GetProxyShapes()[0].get();
        ProxyShape* shapeB = colliderB->GetProxyShapes()[0].get();

        SphereShape* sphereA = dynamic_cast<SphereShape*>(shapeA->GetCollisionShape());
        SphereShape* sphereB = dynamic_cast<SphereShape*>(shapeB->GetCollisionShape());
        if (!sphereA || !sphereB) return false;

        glm::vec3 posA = shapeA->GetWorldPosition();
        glm::vec3 posB = shapeB->GetWorldPosition();

        float radiusA = sphereA->GetRadius();
        float radiusB = sphereB->GetRadius();
        float radiusSum = radiusA + radiusB;

        glm::vec3 delta = posB - posA;
        float distSq = glm::length2(delta);

        if (distSq >= radiusSum * radiusSum)
            return false;

        float distance = glm::sqrt(distSq);
        glm::vec3 normal = (distance > 0.0001f) ? delta / distance : glm::vec3(1, 0, 0);
        float penetration = radiusSum - distance;

        glm::vec3 contactPoint = posA + normal * radiusA;

        manifold.colliderA = colliderA;
        manifold.colliderB = colliderB;
        manifold.contactNormal = normal;
        manifold.penetrationDepth = penetration;
        manifold.contactPoints.clear();
        manifold.contactPoints.push_back(contactPoint);

        return true;
    }

}