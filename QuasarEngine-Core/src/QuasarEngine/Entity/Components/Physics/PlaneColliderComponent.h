#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class PlaneColliderComponent : public PrimitiveColliderComponent
    {
    public:
        PlaneColliderComponent();
        ~PlaneColliderComponent() override;

        PlaneColliderComponent(const PlaneColliderComponent&) = delete;
        PlaneColliderComponent& operator=(const PlaneColliderComponent&) = delete;
        PlaneColliderComponent(PlaneColliderComponent&&) = default;
        PlaneColliderComponent& operator=(PlaneColliderComponent&&) = default;

        void Init() override;
        void UpdateColliderMaterial() override;
        void UpdateColliderSize() override;

        bool  m_UseEntityOrientation = true;

        glm::vec3 m_Normal = { 0.f, 1.f, 0.f };
        float     m_Distance = 0.f;

        physx::PxShape* GetShape()    const noexcept { return mShape; }
        physx::PxMaterial* GetMaterial() const noexcept { return mMaterial; }

    private:
        void AttachOrRebuild();
        physx::PxTransform MakeLocalPose() const;

        physx::PxShape* mShape = nullptr;
        physx::PxMaterial* mMaterial = nullptr;
    };
}
