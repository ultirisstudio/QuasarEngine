#include "qepch.h"
/*#include "Raycast.h"
#include "RaycastUtils.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Entity/Components/Physics/BoxColliderComponent.h>
#include <QuasarEngine/Physic/Shape/BoxShape.h>
#include <QuasarEngine/Entity/Components/IDComponent.h>
#include <QuasarEngine/Scene/Scene.h>
#include <QuasarEngine/Physic/Shape/CollisionShape.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/RaycastUtils.h>
#include <glm/gtx/quaternion.hpp>
#include "QuasarEngine/Physic/RaycastCallback.h"

namespace QuasarEngine {

    RaycastInfo Raycast::RaycastAll(Scene* scene, const glm::vec3& origin, const glm::vec3& direction, float maxDistance) {
        RaycastInfo result;
        result.Hit = false;
        result.Distance = maxDistance;

        RRay ray{ origin, glm::normalize(direction) };

        for (auto e : scene->GetAllEntitiesWith<IDComponent, RigidBodyComponent, BoxColliderComponent>()) {
            Entity entity{ e, scene->GetRegistry() };

            auto& rb = entity.GetComponent<RigidBodyComponent>();
            auto* collider = rb.GetCollider();
            if (!collider) continue;

            auto& proxyShapes = collider->GetProxyShapes();
            if (proxyShapes.empty()) continue;

            auto* shape = proxyShapes[0]->GetCollisionShape();
            if (shape->GetType() != CollisionShapeType::Box) continue;

            auto* boxShape = static_cast<BoxShape*>(shape);

            glm::vec3 boxHalfExtents = boxShape->GetHalfExtents();
            glm::vec3 boxCenter = proxyShapes[0]->ComputeWorldPosition();
            glm::mat3 boxRotation = glm::mat3_cast(proxyShapes[0]->ComputeWorldOrientation());

            float hitDistance;
            glm::vec3 hitPoint, hitNormal;

            if (RayIntersectsOBB(ray, boxCenter, boxRotation, boxHalfExtents, hitDistance, hitPoint, hitNormal)) {
                if (hitDistance < result.Distance) {
                    result.Hit = true;
                    result.Distance = hitDistance;
                    result.HitPoint = hitPoint;
                    result.Normal = hitNormal;
                    result.Entity = entity.GetUUID();
                }
            }
        }

        return result;
    }

    void Raycast::RaycastWithCallback(Scene* scene, const glm::vec3& origin, const glm::vec3& direction, RaycastCallback* callback, float maxDistance) {
        if (!callback) return;

        float currentMaxDistance = maxDistance;
        RRay ray{ origin, glm::normalize(direction) };

        for (auto e : scene->GetAllEntitiesWith<IDComponent, RigidBodyComponent, BoxColliderComponent>()) {
            Entity entity{ e, scene->GetRegistry() };

            auto& rb = entity.GetComponent<RigidBodyComponent>();
            Collider* collider = rb.GetCollider();
            if (!collider) continue;

            auto& proxyShapes = collider->GetProxyShapes();
            if (proxyShapes.empty()) continue;

            for (const auto& proxyPtr : proxyShapes) {
                ProxyShape* proxy = proxyPtr.get();
                CollisionShape* shape = proxy->GetCollisionShape();
                if (!shape || shape->GetType() != CollisionShapeType::Box)
                    continue;

                BoxShape* boxShape = static_cast<BoxShape*>(shape);

                glm::vec3 boxHalfExtents = boxShape->GetHalfExtents();
                glm::vec3 boxCenter = proxy->ComputeWorldPosition();
                glm::mat3 boxRotation = glm::mat3_cast(proxy->ComputeWorldOrientation());

                float hitDistance;
                glm::vec3 hitPoint, hitNormal;

                if (RayIntersectsOBB(ray, boxCenter, boxRotation, boxHalfExtents, hitDistance, hitPoint, hitNormal)) {
                    if (hitDistance > 0.0f && hitDistance <= currentMaxDistance) {
                        RaycastCallback::RaycastHit hit;
                        hit.collider = collider;
                        hit.hitFraction = hitDistance / maxDistance;
                        hit.point = hitPoint;
                        hit.normal = hitNormal;

                        auto result = callback->NotifyRaycastHit(hit);

                        if (result == RaycastCallback::RaycastResult::HitAndStop)
                            return;
                        else if (result == RaycastCallback::RaycastResult::ClipRay)
                            currentMaxDistance = hitDistance;
                        else if (result == RaycastCallback::RaycastResult::IgnoreCollider)
                            continue;
                    }
                }
            }
        }
    }


}*/