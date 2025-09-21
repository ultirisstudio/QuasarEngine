#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>

#include <PxPhysicsAPI.h>

namespace QuasarEngine
{
    class CapsuleColliderComponent : public PrimitiveColliderComponent
    {
    public:
        enum class Axis { X, Y, Z };

        CapsuleColliderComponent();
        ~CapsuleColliderComponent() override;

        CapsuleColliderComponent(const CapsuleColliderComponent&) = delete;
        CapsuleColliderComponent& operator=(const CapsuleColliderComponent&) = delete;
        CapsuleColliderComponent(CapsuleColliderComponent&&) = default;
        CapsuleColliderComponent& operator=(CapsuleColliderComponent&&) = default;

        void Init() override;
        void UpdateColliderMaterial() override;
        void UpdateColliderSize() override;

        bool  m_UseEntityScale = true;
        float m_Radius = 0.5f;
        float m_Height = 1.0f;
        Axis  m_Axis = Axis::Y;

        physx::PxShape* GetShape()    const noexcept { return mShape; }
        physx::PxMaterial* GetMaterial() const noexcept { return mMaterial; }

    private:
        void AttachOrRebuild();
        void RecomputeMassFromSize();

        physx::PxShape* mShape = nullptr;
        physx::PxMaterial* mMaterial = nullptr;
    };
}