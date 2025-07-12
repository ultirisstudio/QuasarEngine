#include "qepch.h"
#include "SphereColliderComponent.h"

#include <QuasarEngine/Entity/Entity.h>
#include <QuasarEngine/Entity/Components/Physics/RigidBodyComponent.h>
#include <QuasarEngine/Entity/Components/TransformComponent.h>
#include <QuasarEngine/Physic/Collision/CollisionDetection.h>
#include <QuasarEngine/Physic/PhysicEngine.h>
#include <QuasarEngine/Physic/RigidBody.h>
#include <QuasarEngine.h>

namespace QuasarEngine {

    SphereColliderComponent::SphereColliderComponent() {
    }

    SphereColliderComponent::~SphereColliderComponent() {
        if (collider) {
            CollisionDetection::UnregisterCollider(collider.get());
        }
    }

    void SphereColliderComponent::Init() {
        Entity entity{ entt_entity, registry };
        auto& tc = entity.GetComponent<TransformComponent>();

        if (entity.HasComponent<RigidBodyComponent>()) {
            auto& rb = entity.GetComponent<RigidBodyComponent>();

            float radius = m_UseEntityScale ? glm::compMax(tc.Scale) * m_Radius : m_Radius;
            std::unique_ptr<SphereShape> shape = std::make_unique<SphereShape>(radius);

            collider = std::make_unique<Collider>(rb.GetRigidBody());
            collider->AddShape(std::move(shape));

            rb.GetRigidBody()->AddCollider(collider.get());

            if (collider) {
                CollisionDetection::RegisterCollider(collider.get());
                UpdateColliderMaterial();
            }
        }
    }

    void SphereColliderComponent::UpdateColliderMaterial() {
        if (!collider) return;

        auto& shapes = collider->GetProxyShapes();
        for (auto& shape : shapes) {
            shape->SetFriction(friction);
            shape->SetBounciness(bounciness);
        }

        Entity entity{ entt_entity, registry };
        if (entity.HasComponent<RigidBodyComponent>()) {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            if (rb.GetRigidBody()) {
                rb.GetRigidBody()->mass = mass;
                rb.GetRigidBody()->inverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
            }
        }
    }

    void SphereColliderComponent::UpdateColliderSize() {
        if (!collider) return;

        Entity entity{ entt_entity, registry };
        float scale = m_UseEntityScale ?
            glm::compMax(entity.GetComponent<TransformComponent>().Scale) :
            1.0f;

        auto& shapes = collider->GetProxyShapes();
        if (!shapes.empty()) {
            shapes.clear();
        }

        float scaledRadius = scale * m_Radius;
        std::unique_ptr<SphereShape> newShape = std::make_unique<SphereShape>(scaledRadius);
        collider->AddShape(std::move(newShape));
    }

}