#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>
#include <glm/glm.hpp>
#include <QuasarEngine/Physic/Shape/SphereShape.h>
#include <QuasarEngine/Physic/Shape/ProxyShape.h>
#include <QuasarEngine/Physic/Collision/Collider.h>

namespace QuasarEngine {

    class SphereColliderComponent : public PrimitiveColliderComponent {
    public:
        SphereColliderComponent();
        ~SphereColliderComponent();

        SphereColliderComponent(const SphereColliderComponent&) = delete;
        SphereColliderComponent& operator=(const SphereColliderComponent&) = delete;

        SphereColliderComponent(SphereColliderComponent&&) = default;
        SphereColliderComponent& operator=(SphereColliderComponent&&) = default;

        void Init() override;
        void UpdateColliderMaterial() override;
        void UpdateColliderSize() override;

        bool& UseEntityScale() { return m_UseEntityScale; }

        float mass = 1.0f;
        float friction = 0.5f;
        float bounciness = 0.2f;
        float linearDamping = 0.01f;
        float angularDamping = 0.05f;

        Collider* GetCollider() const { return collider.get(); }

        float& Radius() { return m_Radius; }
        const float& Radius() const { return m_Radius; }
    private:
        float m_Radius = 0.1f;
        bool m_UseEntityScale = true;

        std::unique_ptr<Collider> collider;
    };

}