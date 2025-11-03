#pragma once

#include <QuasarEngine/Entity/Components/Physics/PrimitiveColliderComponent.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

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

        void SetQueryFilter(uint32_t layer, uint32_t mask);

        void SetTrigger(bool isTrigger);
        bool IsTrigger() const noexcept { return m_IsTrigger; }
        void SetLocalPose(const glm::vec3& localPosition, const glm::quat& localRotation);
        glm::vec3 GetLocalPosition() const noexcept { return m_LocalPosition; }
        glm::quat GetLocalRotation() const noexcept { return m_LocalRotation; }
        void SetMaterialCombineModes(physx::PxCombineMode::Enum friction,
            physx::PxCombineMode::Enum restitution);

        bool  m_UseEntityOrientation = true;

        glm::vec3 m_Normal = { 0.f, 1.f, 0.f };
        float     m_Distance = 0.f;

        physx::PxShape* GetShape()    const noexcept { return m_Shape; }
        physx::PxMaterial* GetMaterial() const noexcept { return m_Material; }

    private:
        void AttachOrRebuild();
        physx::PxTransform MakeLocalPose() const;

        physx::PxShape* m_Shape = nullptr;
        physx::PxMaterial* m_Material = nullptr;

        bool m_IsTrigger = false;

        glm::vec3 m_LocalPosition{ 0 };
        glm::quat m_LocalRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        physx::PxCombineMode::Enum m_FrictionCombine = physx::PxCombineMode::eAVERAGE;
        physx::PxCombineMode::Enum m_RestitutionCombine = physx::PxCombineMode::eAVERAGE;
    };
}
