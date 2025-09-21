#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class SphereColliderComponent : public PrimitiveColliderComponent
    {
    public:
        SphereColliderComponent();
        ~SphereColliderComponent() override;

        SphereColliderComponent(const SphereColliderComponent&) = delete;
        SphereColliderComponent& operator=(const SphereColliderComponent&) = delete;
        SphereColliderComponent(SphereColliderComponent&&) = default;
        SphereColliderComponent& operator=(SphereColliderComponent&&) = default;

        void Init() override;
        void UpdateColliderMaterial() override;
        void UpdateColliderSize() override;

        bool        m_UseEntityScale = true;
        float       m_Radius = 0.5f;

        physx::PxShape* GetShape()    const noexcept { return mShape; }
        physx::PxMaterial* GetMaterial() const noexcept { return mMaterial; }

    private:
        void AttachOrRebuild();
        void RecomputeMassFromSize();

        physx::PxShape* mShape = nullptr;
        physx::PxMaterial* mMaterial = nullptr;
    };
}